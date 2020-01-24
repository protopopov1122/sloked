/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

    class DocumentUpdateListener : public SlokedTransactionStreamListener {
     public:
        DocumentUpdateListener(bool init = true)
            : updated(init) {}

        bool HasUpdates() const {
            return this->updated;
        }

        void ResetUpdates() {
            this->updated = false;
        }

        void OnCommit(const SlokedCursorTransaction &) override {
            this->updated = true;
        }

        void OnRollback(const SlokedCursorTransaction &) override {
            this->updated = true;
        }

        void OnRevert(const SlokedCursorTransaction &) override {
            this->updated = true;
        }

     private:
        bool updated;
    };

    class SlokedTextRenderContext : public SlokedServiceContext {
     public:
        SlokedTextRenderContext(std::unique_ptr<KgrPipe> pipe,
            SlokedEditorDocumentSet &documents, const SlokedCharWidth &charWidth)
            : SlokedServiceContext(std::move(pipe)), documents(documents), charWidth(charWidth), handle(documents.Empty()), document(nullptr) {
                
            this->BindMethod("attach", &SlokedTextRenderContext::Attach);
            this->BindMethod("render", &SlokedTextRenderContext::Render);
        }

     protected:
        void Attach(const std::string &method, const KgrValue &params, Response &rsp) {
            auto doc = this->documents.OpenDocument(static_cast<SlokedEditorDocumentSet::DocumentId>(params.AsDictionary()["document"].AsInt()));
            if (doc.has_value()) {
                std::optional<std::string> externalUri;
                if (doc.value().Exists() && doc.value().GetObject().HasUpstream()) {
                    externalUri = doc.value().GetObject().GetUpstreamURI();
                }
                this->handle = std::move(doc.value());
                this->document = std::make_unique<DocumentContent>(this->handle.GetObject(), this->charWidth);
            }
        }

        void Render(const std::string &method, const KgrValue &params, Response &rsp) {
            const auto &dim = params.AsDictionary();
            auto line = static_cast<TextPosition::Line>(dim["line"].AsInt());
            auto column = static_cast<TextPosition::Column>(dim["column"].AsInt());
            auto updated = this->document->frame.Update(TextPosition{
                    static_cast<TextPosition::Line>(dim["height"].AsInt()),
                    static_cast<TextPosition::Column>(dim["width"].AsInt())
                }, TextPosition{
                    line,
                    column
                }, this->document->updateListener->HasUpdates() || this->document->taggersUpdated.exchange(false));
            this->document->updateListener->ResetUpdates();

            if (updated) {
                KgrArray fragments;
                std::optional<std::pair<std::string, const TaggedTextFragment<int> *>> back;
                std::string newline = this->document->conv.Convert("\n");
                this->document->frame.VisitSymbols([&](auto lineNumber, auto columnOffset, const auto &line) {
                    for (std::size_t column = 0; column < line.size(); column++) {
                        auto tag = this->document->tags.Get(TextPosition {
                            static_cast<TextPosition::Line>(lineNumber),
                            static_cast<TextPosition::Column>(columnOffset + column)
                        });
                        if (!back.has_value()) {
                            back = std::make_pair(line.at(column), tag);
                        } else if (back.value().second != tag) {
                            fragments.Append(KgrDictionary {
                                { "tag", back.value().second != nullptr },
                                { "content", KgrValue(this->document->conv.ReverseConvert(std::move(back.value().first))) }
                            });
                            back = std::make_pair(line.at(column), tag);
                        } else {
                            back.value().first.append(line.at(column));
                        }
                    }
                    
                    if (lineNumber + 1 < this->document->text.GetLastLine()) {
                        if (!back.has_value()) {
                            back = std::make_pair(newline, nullptr);
                        } else if (back.value().second != nullptr) {
                            fragments.Append(KgrDictionary {
                                { "tag", back.value().second != nullptr },
                                { "content", KgrValue(this->document->conv.ReverseConvert(std::move(back.value().first))) }
                            });
                            back = std::make_pair(newline, nullptr);
                        } else {
                            back.value().first.append(newline);
                        }
                    }

                });
                if (back.has_value()) {
                    fragments.Append(KgrDictionary {
                        { "tag", back.value().second != nullptr },
                        { "content", KgrValue(this->document->conv.ReverseConvert(std::move(back.value().first))) }
                    });
                }
                this->rendered = std::move(fragments);
            }
            
            auto realLine = this->document->text.GetLine(line);
            auto realPos = this->charWidth.GetRealPosition(realLine, column, this->document->conv.GetDestination());
            auto realColumn = column < this->document->conv.GetDestination().CodepointCount(realLine) ? realPos.first : realPos.second;
            const auto &offset = this->document->frame.GetOffset();
            KgrDictionary cursor {
                { "line", static_cast<int64_t>(line - offset.line) },
                { "column", static_cast<int64_t>(realColumn - offset.column) }
            };
            rsp.Result(KgrDictionary {
                { "cursor", std::move(cursor) },
                { "content", this->rendered }
            });
        }

     private:

        struct DocumentContent {
            DocumentContent(SlokedEditorDocument &document, const SlokedCharWidth &charWidth)
                : text(document.GetText()), conv(SlokedLocale::SystemEncoding(), document.GetEncoding()),
                  transactionListeners(document.GetTransactionListeners()), lazyTags(&document.GetTagger()),
                  tags(lazyTags), frame(document.GetText(), document.GetEncoding(), charWidth), taggersUpdated{false} {

                this->fragmentUpdater = std::make_shared<SlokedFragmentUpdater<int>>(this->text, this->tags, this->conv.GetDestination(), charWidth);
                this->updateListener = std::make_shared<DocumentUpdateListener>();
                this->transactionListeners.AddListener(this->fragmentUpdater);
                this->transactionListeners.AddListener(this->updateListener);
                this->unsubscribeTaggers = document.GetTagger().OnUpdate([this](const auto &) {
                    this->taggersUpdated = true;
                });
            }

            ~DocumentContent() {
                this->transactionListeners.RemoveListener(*this->fragmentUpdater);
                if (this->unsubscribeTaggers) {
                    this->unsubscribeTaggers();
                }
            }
                
            TextBlock &text;
            EncodingConverter conv;
            SlokedTransactionListenerManager &transactionListeners;
            SlokedLazyTaggedText<int> lazyTags;
            SlokedCacheTaggedText<int> tags;
            TextFrameView frame;
            std::shared_ptr<SlokedFragmentUpdater<int>> fragmentUpdater;
            std::shared_ptr<DocumentUpdateListener> updateListener;
            SlokedTextTagger<SlokedEditorDocument::TagType>::Unbind unsubscribeTaggers;
            std::atomic<bool> taggersUpdated;
        };

        SlokedEditorDocumentSet &documents;
        const SlokedCharWidth &charWidth;
        SlokedEditorDocumentSet::Document handle;
        std::unique_ptr<DocumentContent> document;
        KgrArray rendered;
    };


    SlokedTextRenderService::SlokedTextRenderService(SlokedEditorDocumentSet &documents, const SlokedCharWidth &charWidth, KgrContextManager<KgrLocalContext> &contextManager)
        : documents(documents), charWidth(charWidth), contextManager(contextManager) {}

    void SlokedTextRenderService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedTextRenderContext>(std::move(pipe), documents, this->charWidth);
        this->contextManager.Attach(std::move(ctx));
    }

    SlokedTextRenderClient::SlokedTextRenderClient(std::unique_ptr<KgrPipe> pipe, SlokedEditorDocumentSet::DocumentId docId)
        : client(std::move(pipe)) {
        auto rsp = this->client.Invoke("attach", KgrDictionary {
            { "document", static_cast<int64_t>(docId) }
        });
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