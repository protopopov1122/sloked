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

#include "sloked/services/TextEditor.h"
#include "sloked/core/Error.h"
#include "sloked/core/Locale.h"
#include "sloked/text/cursor/TransactionCursor.h"
#include "sloked/text/cursor/TransactionJournal.h"
#include "sloked/text/fragment/Updater.h"
#include "sloked/text/TextFrame.h"

namespace sloked {

    class TextCharWidthIterator {
     public:
        TextCharWidthIterator(std::string_view line, TextPosition::Column offset, const Encoding &encoding, const SlokedCharWidth &charWidth)
            : line(line), column(0), offset(offset), encoding(encoding), charWidth(charWidth) {
            this->line_codepoints = this->encoding.CodepointCount(this->line);
            this->line_real_length = this->charWidth.GetRealPosition(this->line, this->line_codepoints, this->encoding).second;
            if  (this->offset < this->line_real_length) {
                this->SeekColumn();
            }
        }

        TextPosition::Column GetRealColumn() const {
            return this->column;
        }

        void Next() {
            if (this->offset++ < this->line_real_length) {
                this->SeekColumn();
            }
        }

     private:
        void SeekColumn() {
            auto realPos = this->charWidth.GetRealPosition(this->line, this->column + 1, this->encoding);
            while (realPos.first <= this->offset && this->column + 1 < this->line_codepoints) {
                realPos = this->charWidth.GetRealPosition(this->line, ++this->column + 1, this->encoding);
            }
        }

        std::string line;
        std::size_t line_codepoints;
        std::size_t line_real_length;
        TextPosition::Column column;
        TextPosition::Column offset;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
    };

    class SlokedTextEditorContext : public KgrLocalContext {
     public:
        SlokedTextEditorContext(std::unique_ptr<KgrPipe> pipe,
            TextBlock &text, const EncodingConverter &conv, const SlokedCharWidth &charWidth,
            std::unique_ptr<SlokedTransactionStream> stream, std::unique_ptr<SlokedTextTagger<int>> tagger)
            : KgrLocalContext(std::move(pipe)),
              text(text), conv(conv), charWidth(charWidth), cursor(text, conv.GetDestination(), *stream),
              lazyTags(std::move(tagger)), tags(lazyTags), frame(text, conv.GetDestination(), charWidth) {
            
            this->stream = std::move(stream);
            auto fragmentUpdater = std::make_shared<SlokedFragmentUpdater<int>>(this->text, this->tags, this->conv.GetDestination(), this->charWidth);
            this->stream->AddListener(fragmentUpdater);
        }

        void Run() override {
            try {
                if (!this->pipe->Empty()) {
                    auto msg = this->pipe->Read();
                    auto res = this->ProcessMessage(msg);
                    if (res.has_value()) {
                        this->pipe->Write(std::move(res.value()));
                    }
                }
            } catch (const SlokedError &ex) {
                if (this->pipe->GetStatus() == KgrPipe::Status::Open) {
                    throw;
                }
            }
        }

     private:
        std::optional<KgrValue> ProcessMessage(const KgrValue &message) {
            const auto &prms = message.AsDictionary();
            auto command = static_cast<SlokedTextEditorService::Command>(prms["command"].AsInt());
            switch (command) {
                case SlokedTextEditorService::Command::Insert:
                    this->cursor.Insert(this->conv.Convert(prms["content"].AsString()));
                    break;

                case SlokedTextEditorService::Command::MoveUp:
                    this->cursor.MoveUp(1);
                    break;

                case SlokedTextEditorService::Command::MoveDown:
                    this->cursor.MoveDown(1);
                    break;

                case SlokedTextEditorService::Command::MoveBackward:
                    this->cursor.MoveBackward(1);
                    break;

                case SlokedTextEditorService::Command::MoveForward:
                    this->cursor.MoveForward(1);
                    break;

                case SlokedTextEditorService::Command::NewLine:
                    this->cursor.NewLine("");
                    break;

                case SlokedTextEditorService::Command::DeleteBackward:
                    this->cursor.DeleteBackward();
                    break;

                case SlokedTextEditorService::Command::DeleteForward:
                    this->cursor.DeleteForward();
                    break;

                case SlokedTextEditorService::Command::Undo:
                    this->cursor.Undo();
                    break;

                case SlokedTextEditorService::Command::Redo:
                    this->cursor.Redo();
                    break;

                case SlokedTextEditorService::Command::Render:
                    return this->Render(prms);
            }
            return std::optional<KgrValue>();
        }

        KgrValue Render(const KgrDictionary &dict) {
            const auto &dim = dict["dim"].AsDictionary();
            this->frame.Update(TextPosition{
                    static_cast<TextPosition::Line>(dim["height"].AsInt()),
                    static_cast<TextPosition::Column>(dim["width"].AsInt())
                }, TextPosition{
                    this->cursor.GetLine(), this->cursor.GetColumn()
                });

            KgrArray fragments;
            TextPosition::Line lineNumber = 0;
            this->frame.Visit(0, std::min(static_cast<TextPosition::Line>(dim["height"].AsInt()), static_cast<TextPosition::Line>(this->frame.GetLastLine()) + 1), [&](const auto lineView) {
                std::string line{lineView};
                std::string fullLine{this->text.GetLine(this->frame.GetOffset().line + lineNumber)};

                const auto lineLength = this->conv.GetDestination().CodepointCount(line);
                std::size_t frameOffset = this->frame.GetOffset().column;
                
                TextCharWidthIterator iter(fullLine, frameOffset, this->conv.GetDestination(), this->charWidth);
                for (TextPosition::Column column = 0; column < lineLength; column++, iter.Next()) {
                    auto tag = this->tags.Get(TextPosition{this->frame.GetOffset().line + lineNumber, iter.GetRealColumn()});
                    auto pos = this->conv.GetDestination().GetCodepoint(line, column);
                    auto fragment = line.substr(pos.first, pos.second);
                    fragments.Append(KgrDictionary {
                        { "tag", tag != nullptr },
                        { "content", KgrValue(conv.ReverseConvert(fragment)) }
                    });
                }
                if (lineNumber++ < this->frame.GetLastLine()) {
                    fragments.Append(KgrDictionary {
                        { "tag", false },
                        { "content", KgrValue("\n") }
                    });
                }
            });
            
            std::string realLine{this->text.GetLine(this->cursor.GetLine())};
            auto realPos = this->charWidth.GetRealPosition(realLine, this->cursor.GetColumn(), conv.GetDestination());
            auto realColumn = this->cursor.GetColumn() < this->conv.GetDestination().CodepointCount(realLine) ? realPos.first : realPos.second;
            const auto &offset = this->frame.GetOffset();
            KgrDictionary cursor {
                { "line", static_cast<int64_t>(this->cursor.GetLine() - offset.line) },
                { "column", static_cast<int64_t>(realColumn - offset.column) }
            };
            return KgrDictionary {
                { "cursor", std::move(cursor) },
                { "content", std::move(fragments) }
            };
        }

        TextBlock &text;
        const EncodingConverter &conv;
        const SlokedCharWidth &charWidth;
        std::unique_ptr<SlokedTransactionStream> stream;
        TransactionCursor cursor;
        SlokedLazyTaggedText<int> lazyTags;
        SlokedCacheTaggedText<int> tags;
        TextFrameView frame;
    };


    SlokedTextEditorService::SlokedTextEditorService(TextBlock &text, const Encoding &encoding, const SlokedCharWidth &charWidth, SlokedTextTaggerFactory<int> &taggerFactory,
        KgrContextManager<KgrLocalContext> &contextManager)
        : text(text), conv(SlokedLocale::SystemEncoding(), encoding), taggerFactory(taggerFactory), charWidth(charWidth), contextManager(contextManager),
          multiplexer(text, encoding) {}

    bool SlokedTextEditorService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedTextEditorContext>(std::move(pipe), this->text, this->conv, this->charWidth, this->multiplexer.NewStream(), this->taggerFactory.Create(this->text, this->conv.GetDestination(), this->charWidth));
        this->contextManager.Attach(std::move(ctx));
        return true;
    }
}