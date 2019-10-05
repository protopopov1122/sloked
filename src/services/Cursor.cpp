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

#include "sloked/services/Cursor.h"
#include "sloked/core/Error.h"
#include "sloked/core/Locale.h"
#include "sloked/text/cursor/TransactionCursor.h"
#include "sloked/text/cursor/TransactionJournal.h"
#include "sloked/text/fragment/Updater.h"
#include "sloked/text/TextFrame.h"

namespace sloked {

    class SlokedCursorContext : public SlokedServiceContext {
     public:
        SlokedCursorContext(std::unique_ptr<KgrPipe> pipe,
            SlokedEditorDocumentSet &documents)
            : SlokedServiceContext(std::move(pipe)), documents(documents), handle(documents.Empty()), document(nullptr) {}

     protected:
        void ProcessRequest(const KgrValue &message) override {
            const auto &prms = message.AsDictionary();
            auto command = static_cast<SlokedCursorService::Command>(prms["command"].AsInt());
            if (command != SlokedCursorService::Command::Connect &&
                this->document == nullptr) {
                return;
            }
            switch (command) {
                case SlokedCursorService::Command::Connect: {
                    auto doc = this->documents.OpenDocument(static_cast<SlokedEditorDocumentSet::DocumentId>(prms["id"].AsInt()));
                    if (doc.has_value()) {
                        this->handle = std::move(doc.value());
                        this->document = std::make_unique<DocumentContent>(this->handle.GetObject());
                        this->SendResponse(true);
                    } else {
                        this->SendResponse(false);
                    }
                } break;
                
                case SlokedCursorService::Command::Insert:
                    this->document->cursor.Insert(this->document->conv.Convert(prms["content"].AsString()));
                    break;

                case SlokedCursorService::Command::MoveUp:
                    this->document->cursor.MoveUp(1);
                    break;

                case SlokedCursorService::Command::MoveDown:
                    this->document->cursor.MoveDown(1);
                    break;

                case SlokedCursorService::Command::MoveBackward:
                    this->document->cursor.MoveBackward(1);
                    break;

                case SlokedCursorService::Command::MoveForward:
                    this->document->cursor.MoveForward(1);
                    break;

                case SlokedCursorService::Command::NewLine:
                    this->document->cursor.NewLine("");
                    break;

                case SlokedCursorService::Command::DeleteBackward:
                    this->document->cursor.DeleteBackward();
                    break;

                case SlokedCursorService::Command::DeleteForward:
                    this->document->cursor.DeleteForward();
                    break;

                case SlokedCursorService::Command::Undo:
                    this->document->cursor.Undo();
                    break;

                case SlokedCursorService::Command::Redo:
                    this->document->cursor.Redo();
                    break;

                case SlokedCursorService::Command::Info:
                    this->SendResponse(KgrDictionary {
                        { "line", static_cast<int64_t>(this->document->cursor.GetLine()) },
                        { "column", static_cast<int64_t>(this->document->cursor.GetColumn()) }
                    });
                    break;
            }
        }

     private:
        struct DocumentContent {
            DocumentContent(SlokedEditorDocument &document)
                : text(document.GetText()), conv(SlokedLocale::SystemEncoding(), document.GetEncoding()),
                  stream(document.NewStream()), cursor(document.GetText(), document.GetEncoding(), *stream) {}
                
            TextBlock &text;
            EncodingConverter conv;
            std::unique_ptr<SlokedTransactionStream> stream;
            TransactionCursor cursor;
        };

        SlokedEditorDocumentSet &documents;
        SlokedEditorDocumentSet::Document handle;
        std::unique_ptr<DocumentContent> document;
    };


    SlokedCursorService::SlokedCursorService(SlokedEditorDocumentSet &documents, KgrContextManager<KgrLocalContext> &contextManager)
        : documents(documents), contextManager(contextManager) {}

    bool SlokedCursorService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedCursorContext>(std::move(pipe), this->documents);
        this->contextManager.Attach(std::move(ctx));
        return true;
    }
}