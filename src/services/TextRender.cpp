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

#include "sloked/services/TextRender.h"
#include "sloked/core/Error.h"
#include "sloked/core/Locale.h"
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

    class SlokedTextRenderContext : public SlokedServiceContext {
     public:
        SlokedTextRenderContext(std::unique_ptr<KgrPipe> pipe,
            TextBlock &text, const EncodingConverter &conv, const SlokedCharWidth &charWidth,
            SlokedTransactionListenerManager &transactionListeners, std::unique_ptr<SlokedTextTagger<int>> tagger)
            : SlokedServiceContext(std::move(pipe)),
              text(text), conv(conv), charWidth(charWidth), transactionListeners(transactionListeners),
              lazyTags(std::move(tagger)), tags(lazyTags), frame(text, conv.GetDestination(), charWidth) {
            this->fragmentUpdater = std::make_shared<SlokedFragmentUpdater<int>>(this->text, this->tags, this->conv.GetDestination(), this->charWidth);
            this->transactionListeners.AddListener(this->fragmentUpdater);
        }

        ~SlokedTextRenderContext() {
            this->transactionListeners.RemoveListener(*this->fragmentUpdater);
        }

     protected:
        void ProcessRequest(const KgrValue &message) override {
            const auto &prms = message.AsDictionary();
            this->SendResponse(this->Render(prms));
        }

     private:
        KgrValue Render(const KgrDictionary &dict) {
            const auto &dim = dict["dim"].AsDictionary();
            auto line = static_cast<TextPosition::Line>(dim["line"].AsInt());
            auto column = static_cast<TextPosition::Column>(dim["column"].AsInt());
            this->frame.Update(TextPosition{
                    static_cast<TextPosition::Line>(dim["height"].AsInt()),
                    static_cast<TextPosition::Column>(dim["width"].AsInt())
                }, TextPosition{
                    line,
                    column
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
            
            std::string realLine{this->text.GetLine(line)};
            auto realPos = this->charWidth.GetRealPosition(realLine, column, conv.GetDestination());
            auto realColumn = column < this->conv.GetDestination().CodepointCount(realLine) ? realPos.first : realPos.second;
            const auto &offset = this->frame.GetOffset();
            KgrDictionary cursor {
                { "line", static_cast<int64_t>(line - offset.line) },
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
        SlokedTransactionListenerManager &transactionListeners;
        SlokedLazyTaggedText<int> lazyTags;
        SlokedCacheTaggedText<int> tags;
        TextFrameView frame;
        std::shared_ptr<SlokedFragmentUpdater<int>> fragmentUpdater;
    };


    SlokedTextRenderService::SlokedTextRenderService(TextBlock &text, const Encoding &encoding, SlokedTransactionListenerManager &transactioListeners, const SlokedCharWidth &charWidth, SlokedTextTaggerFactory<int> &taggerFactory,
        KgrContextManager<KgrLocalContext> &contextManager)
        : text(text), conv(SlokedLocale::SystemEncoding(), encoding), transactionListeners(transactioListeners), taggerFactory(taggerFactory), charWidth(charWidth), contextManager(contextManager) {}

    bool SlokedTextRenderService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedTextRenderContext>(std::move(pipe), this->text, this->conv, this->charWidth, this->transactionListeners, this->taggerFactory.Create(this->text, this->conv.GetDestination(), this->charWidth));
        this->contextManager.Attach(std::move(ctx));
        return true;
    }
}