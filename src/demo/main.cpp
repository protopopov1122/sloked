/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include <iostream>
#include "sloked/core/Locale.h"
#include "sloked/screen/terminal/posix/PosixTerminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalBuffer.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include "sloked/screen/terminal/components/SplitterComponent.h"
#include "sloked/screen/terminal/components/TabberComponent.h"
#include "sloked/screen/terminal/components/MultiplexerComponent.h"
#include "sloked/screen/widgets/TextEditor.h"
#include "sloked/namespace/Filesystem.h"
#include "sloked/namespace/Virtual.h"
#include "sloked/namespace/posix/Filesystem.h"
#include "sloked/text/fragment/TaggedText.h"
#include "sloked/kgr/local/Pipe.h"
#include "sloked/kgr/local/Server.h"
#include "sloked/kgr/local/NamedServer.h"
#include "sloked/kgr/ctx-manager/RunnableContextManager.h"
#include "sloked/services/TextRender.h"
#include "sloked/services/Cursor.h"
#include "sloked/services/DocumentSet.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/editor/Tabs.h"
#include "sloked/screen/components/ComponentTree.h"

using namespace sloked;

class TestFragment : public SlokedTextTagger<int> {
 public:
    TestFragment(const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth)
        : text(text), encoding(encoding), charWidth(charWidth), current{0, 0} {}

    std::optional<TaggedTextFragment<int>> Next() override {
        if (this->cache.empty()) {
            this->ParseLine();
        }
        if (!this->cache.empty()) {
            auto fragment = std::move(this->cache.front());
            this->cache.pop();
            return fragment;
        } else {
            return {};
        }
    }
    
    void Rewind(const TextPosition &position) override {
        if (position < this->current) {
            this->current = position;
            this->cache = {};
        }
    }

    const TextPosition &GetPosition() const override {
        return this->current;
    }

 private:
    bool ParseLine() {
        if (this->current.line > this->text.GetLastLine()) {
            return false;
        }

        std::string currentLine{this->text.GetLine(this->current.line)};
        currentLine.erase(0, this->encoding.GetCodepoint(currentLine, this->current.column).first);
        this->encoding.IterateCodepoints(currentLine, [&](auto start, auto length, auto chr) {
            if (chr == U'\t') {
                this->cache.push(TaggedTextFragment<int>(this->current, TextPosition{0, static_cast<TextPosition::Column>(1)}, 1));
            }
            this->current.column++;
            return true;
        });

        this->current.line++;
        this->current.column = 0;
        return true;
    }

    const TextBlockView &text;
    const Encoding &encoding;
    const SlokedCharWidth &charWidth;
    TextPosition current;
    std::queue<TaggedTextFragment<int>> cache;
};

class TestFragmentFactory : public SlokedTextTaggerFactory<int> {
 public:
    std::unique_ptr<SlokedTextTagger<int>> Create(const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth) const override {
        return std::make_unique<TestFragment>(text, encoding, charWidth);
    }
};

int main(int argc, const char **argv) {
    if (argc < 3) {
        std::cout << "Format: " << argv[0] << " source destination" << std::endl;
        return EXIT_FAILURE;
    }

    KgrLocalServer portServer;
    KgrLocalNamedServer server(portServer);
    KgrRunnableContextManagerHandle<KgrLocalContext> ctxManagerHandle;
    KgrContextManager<KgrLocalContext> &ctxManager = ctxManagerHandle.GetManager();
    ctxManagerHandle.Start();

    char INPUT_PATH[1024], OUTPUT_PATH[1024];
    realpath(argv[1], INPUT_PATH);
    realpath(argv[2], OUTPUT_PATH);
    SlokedVirtualNamespace root(std::make_unique<SlokedFilesystemNamespace>(std::make_unique<SlokedPosixFilesystemAdapter>("/")));
    SlokedEditorDocumentSet documents(root);

    SlokedLocale::Setup();
    const Encoding &terminalEncoding = Encoding::Get("system");

    SlokedCharWidth charWidth;
    TestFragmentFactory fragmentFactory;

    server.Register("text::render", std::make_unique<SlokedTextRenderService>(documents, charWidth, fragmentFactory, ctxManager));
    server.Register("text::cursor", std::make_unique<SlokedCursorService>(documents, ctxManager));
    server.Register("documents", std::make_unique<SlokedDocumentSetService>(documents, ctxManager));

    PosixTerminal terminal;
    BufferedTerminal console(terminal, terminalEncoding, charWidth);

    TerminalComponentHandle screen(console, terminalEncoding, charWidth);
    auto &multi = screen.NewMultiplexer();
    auto mainWindow = multi.NewWindow(TextPosition{0, 0}, TextPosition{console.GetHeight(), console.GetWidth()});
    auto &splitter = mainWindow->GetComponent().NewSplitter(Splitter::Direction::Vertical);
    auto tabberWindow = splitter.NewWindow(Splitter::Constraints(1.0f));
    auto cmdlineWindow = splitter.NewWindow(Splitter::Constraints(0.0f, 1));
    tabberWindow->GetComponent().NewTabber();
    SlokedEditorTabs tabs(SlokedComponentTree::Traverse(screen, "/0/0/self").AsTabber(), terminalEncoding, server.GetConnector("text::cursor"), server.GetConnector("text::render"), server.GetConnector("documents"));

    auto tab = tabs.Open(INPUT_PATH, "system", "system");

    bool work = true;
    screen.SetInputHandler([&](const SlokedKeyboardInput &cmd) {
        if (cmd.value.index() == 0) {
            return false;
        } else switch (std::get<1>(cmd.value)) {            
            case SlokedControlKey::Escape: {
                tab->Save(OUTPUT_PATH);
                tab->Close();
                work = false;
            } break;

            default:
                return false;
        }
        return true;
    });

    while (work) {
        screen.UpdateDimensions();
        console.SetGraphicsMode(SlokedTextGraphics::Off);
        console.ClearScreen();
        screen.Render();
        console.Flush();
        auto input = terminal.GetInput();
        for (const auto &evt : input) {
            screen.ProcessInput(evt);
        }
    }
    tabs.CloseAll();

    ctxManagerHandle.Stop();
    return EXIT_SUCCESS;
}