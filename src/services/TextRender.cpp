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
            SlokedEditorDocumentSet &documents, const SlokedCharWidth &charWidth, SlokedTextTaggerFactory<int> &taggerFactory)
            : SlokedServiceContext(std::move(pipe)), documents(documents), charWidth(charWidth), taggerFactory(taggerFactory), handle(documents.Empty()), document(nullptr) {
                
            this->RegisterMethod("attach", [this](const std::string &method, const KgrValue &params, Response &rsp) { this->Attach(method, params, rsp); });
            this->RegisterMethod("render", [this](const std::string &method, const KgrValue &params, Response &rsp) { this->Render(method, params, rsp); });
        }

     protected:
        void Attach(const std::string &method, const KgrValue &params, Response &rsp) {
            auto doc = this->documents.OpenDocument(static_cast<SlokedEditorDocumentSet::DocumentId>(params.AsInt()));
            if (doc.has_value()) {
                this->handle = std::move(doc.value());
                this->document = std::make_unique<DocumentContent>(this->handle.GetObject(), this->charWidth, this->taggerFactory);
            }
        }

        void Render(const std::string &method, const KgrValue &params, Response &rsp) {
            const auto &dim = params.AsDictionary();
            auto line = static_cast<TextPosition::Line>(dim["line"].AsInt());
            auto column = static_cast<TextPosition::Column>(dim["column"].AsInt());
            this->document->frame.Update(TextPosition{
                    static_cast<TextPosition::Line>(dim["height"].AsInt()),
                    static_cast<TextPosition::Column>(dim["width"].AsInt())
                }, TextPosition{
                    line,
                    column
                });

            KgrArray fragments;
            TextPosition::Line lineNumber = 0;
            this->document->frame.Visit(0, std::min(static_cast<TextPosition::Line>(dim["height"].AsInt()), static_cast<TextPosition::Line>(this->document->frame.GetLastLine()) + 1), [&](const auto lineView) {
                std::string line{lineView};
                std::string fullLine{this->document->text.GetLine(this->document->frame.GetOffset().line + lineNumber)};

                const auto lineLength = this->document->conv.GetDestination().CodepointCount(line);
                std::size_t frameOffset = this->document->frame.GetOffset().column;
                
                TextCharWidthIterator iter(fullLine, frameOffset, this->document->conv.GetDestination(), this->charWidth);
                for (TextPosition::Column column = 0; column < lineLength; column++, iter.Next()) {
                    auto tag = this->document->tags.Get(TextPosition{this->document->frame.GetOffset().line + lineNumber, iter.GetRealColumn()});
                    auto pos = this->document->conv.GetDestination().GetCodepoint(line, column);
                    auto fragment = line.substr(pos.first, pos.second);
                    fragments.Append(KgrDictionary {
                        { "tag", tag != nullptr },
                        { "content", KgrValue(this->document->conv.ReverseConvert(fragment)) }
                    });
                }
                if (lineNumber++ < this->document->frame.GetLastLine()) {
                    fragments.Append(KgrDictionary {
                        { "tag", false },
                        { "content", KgrValue("\n") }
                    });
                }
            });
            
            std::string realLine{this->document->text.GetLine(line)};
            auto realPos = this->charWidth.GetRealPosition(realLine, column, this->document->conv.GetDestination());
            auto realColumn = column < this->document->conv.GetDestination().CodepointCount(realLine) ? realPos.first : realPos.second;
            const auto &offset = this->document->frame.GetOffset();
            KgrDictionary cursor {
                { "line", static_cast<int64_t>(line - offset.line) },
                { "column", static_cast<int64_t>(realColumn - offset.column) }
            };
            rsp.Result(KgrDictionary {
                { "cursor", std::move(cursor) },
                { "content", std::move(fragments) }
            });
        }

     private:

        struct DocumentContent {
            DocumentContent(SlokedEditorDocument &document, const SlokedCharWidth &charWidth, SlokedTextTaggerFactory<int> &taggerFactory)
                : text(document.GetText()), conv(SlokedLocale::SystemEncoding(), document.GetEncoding()),
                  transactionListeners(document.GetTransactionListeners()), lazyTags(taggerFactory.Create(document.GetText(), document.GetEncoding(), charWidth)),
                  tags(lazyTags), frame(document.GetText(), document.GetEncoding(), charWidth) {

                this->fragmentUpdater = std::make_shared<SlokedFragmentUpdater<int>>(this->text, this->tags, this->conv.GetDestination(), charWidth);
                this->transactionListeners.AddListener(this->fragmentUpdater);
            }

            ~DocumentContent() {
                this->transactionListeners.RemoveListener(*this->fragmentUpdater);
            }
                
            TextBlock &text;
            EncodingConverter conv;
            SlokedTransactionListenerManager &transactionListeners;
            SlokedLazyTaggedText<int> lazyTags;
            SlokedCacheTaggedText<int> tags;
            TextFrameView frame;
            std::shared_ptr<SlokedFragmentUpdater<int>> fragmentUpdater;
        };

        SlokedEditorDocumentSet &documents;
        const SlokedCharWidth &charWidth;
        SlokedTextTaggerFactory<int> &taggerFactory;
        SlokedEditorDocumentSet::Document handle;
        std::unique_ptr<DocumentContent> document;
    };


    SlokedTextRenderService::SlokedTextRenderService(SlokedEditorDocumentSet &documents, const SlokedCharWidth &charWidth, SlokedTextTaggerFactory<int> &taggerFactory,
        KgrContextManager<KgrLocalContext> &contextManager)
        : documents(documents), taggerFactory(taggerFactory), charWidth(charWidth), contextManager(contextManager) {}

    bool SlokedTextRenderService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedTextRenderContext>(std::move(pipe), documents, this->charWidth, this->taggerFactory);
        this->contextManager.Attach(std::move(ctx));
        return true;
    }

    SlokedTextRenderClient::SlokedTextRenderClient(std::unique_ptr<KgrPipe> pipe, SlokedEditorDocumentSet::DocumentId docId)
        : client(std::move(pipe)) {
        auto rsp = this->client.Invoke("attach", static_cast<int64_t>(docId));
    }

    std::optional<KgrValue> SlokedTextRenderClient::Render(const TextPosition &pos, const TextPosition &dim) {
        auto rsp = this->client.Invoke("render", KgrDictionary {
            { "height", static_cast<int64_t>(dim.line) },
            { "width", static_cast<int64_t>(dim.column) },
            { "line", static_cast<int64_t>(pos.line) },
            { "column", static_cast<int64_t>(pos.column) }
        });
        auto renderRes = rsp.Get();
        if (!renderRes.HasResult()) {
            return {};
        }
        return std::move(renderRes.GetResult());
    }
}