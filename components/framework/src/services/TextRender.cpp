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
#include "sloked/core/Cache.h"

namespace sloked {

    class DocumentUpdateListener : public SlokedTransactionStreamListener {
     public:
        DocumentUpdateListener()
            : lastUpdate{{0, 0}} {}

        bool HasUpdates() {
            std::unique_lock lock(this->mtx);
            return this->lastUpdate.has_value();
        }

        std::optional<TextPosition> GetUpdate() {
            std::unique_lock lock(this->mtx);
            auto res = std::move(this->lastUpdate);
            this->lastUpdate = {};
            return res;
        }

        void OnCommit(const SlokedCursorTransaction &trans) final {
            std::unique_lock lock(this->mtx);
            this->lastUpdate = trans.GetPosition();
        }

        void OnRollback(const SlokedCursorTransaction &trans) final {
            std::unique_lock lock(this->mtx);
            this->lastUpdate = trans.GetPosition();
        }

        void OnRevert(const SlokedCursorTransaction &trans) final {
            std::unique_lock lock(this->mtx);
            this->lastUpdate = trans.GetPosition();
        }

     private:
        std::mutex mtx;
        std::optional<TextPosition> lastUpdate;
    };

    class SlokedTextRenderContext : public SlokedServiceContext {
     public:
        SlokedTextRenderContext(std::unique_ptr<KgrPipe> pipe,
            SlokedEditorDocumentSet &documents, const SlokedCharPreset &charPreset)
            : SlokedServiceContext(std::move(pipe)), documents(documents), charPreset(charPreset), handle(documents.Empty()), document(nullptr),
              cache([this](const auto &begin, const auto &end) { return this->RenderLines(begin, end); }) {
                
            this->BindMethod("attach", &SlokedTextRenderContext::Attach);
            this->BindMethod("realPosition", &SlokedTextRenderContext::RealPosition);
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
                this->document = std::make_unique<DocumentContent>(this->handle.GetObject(), this->charPreset);
            }
        }

        void RealPosition(const std::string &method, const KgrValue &params, Response &rsp) {
            TextPosition::Line line = params.AsDictionary()["line"].AsInt();
            TextPosition::Column column = params.AsDictionary()["column"].AsInt();
            auto realLine = this->document->text.GetLine(line);
            auto realColumnPos = this->charPreset.GetRealPosition(realLine, column, this->document->encoding);
            auto realColumn = column < this->document->encoding.CodepointCount(realLine) ?
                realColumnPos.first : realColumnPos.second;
            rsp.Result(KgrDictionary {
                { "line", static_cast<int64_t>(line) },
                { "column", static_cast<int64_t>(realColumn) }
            });
        }

        void Render(const std::string &method, const KgrValue &params, Response &rsp) {
            const auto &dim = params.AsDictionary();
            auto lineNumber = static_cast<TextPosition::Line>(dim["line"].AsInt());
            auto height = static_cast<TextPosition::Line>(dim["height"].AsInt());

            auto lastUpdate = this->document->updateListener->GetUpdate();
            if (this->document->taggersUpdated.exchange(false)) {
                this->cache.Clear();
            } else if (lastUpdate.has_value()) {
                if (lastUpdate.value().line > 0) {
                    this->cache.Drop(lastUpdate.value().line - 1, TextPosition::Max.line);
                } else {
                    this->cache.Clear();
                }
            }

            KgrArray lines;
            auto [begin, end] = this->cache.Fetch(lineNumber, std::min(lineNumber + height, static_cast<TextPosition::Line>(this->document->text.GetLastLine())));
            for (auto it = begin; it != end; ++it) {
                lines.Append(it->second);
            }
            
            rsp.Result(std::move(lines));
        }

     private:
        std::vector<KgrValue> RenderLines(const TextPosition::Line &begin, const TextPosition::Line &end) {
            std::vector<KgrValue> result;
            auto lineIdx = begin;
            const std::string tab = this->charPreset.GetTab(this->document->encoding);
            this->document->text.Visit(begin, std::min(end - begin + 1,  static_cast<TextPosition::Line>(this->document->text.GetLastLine() - begin + 1)), [&](auto line) {
                std::optional<std::pair<std::string, std::optional<TaggedTextFragment<int>>>> back;
                KgrArray fragments;
                TextPosition::Column columnIdx{0};
                auto lineTags = this->document->tags.Get(lineIdx);
                std::string buffer{};
                buffer.reserve(line.size());
                const TaggedTextFragment<int> *current{nullptr};
                auto iter = lineTags.begin();
                const auto length = line.size();
                for (Encoding::Iterator it{}; (it = this->document->encoding.Iterate(it, line, length)).value != U'\0';) {
                    if (current != nullptr && current->GetEnd().column <= columnIdx ) {
                        if (!buffer.empty()) {
                            fragments.Append(KgrDictionary {
                                { "tag", current != nullptr },
                                { "content", this->document->conv.ReverseConvert(buffer) }
                            });
                            buffer.clear();
                        }
                        current = nullptr;
                    }
                    if (iter != lineTags.end() && iter->GetStart().column == columnIdx) {
                        if (!buffer.empty()) {
                            fragments.Append(KgrDictionary {
                                { "tag", current != nullptr },
                                { "content", this->document->conv.ReverseConvert(buffer) }
                            });
                            buffer.clear();
                        }
                        current = std::addressof(*iter);
                        ++iter;
                    }
                    std::string_view fragment = it.value != U'\t' ? line.substr(it.start, it.length) : tab;
                    buffer += fragment;
                    columnIdx++;
                }
                if (!buffer.empty()) {
                    fragments.Append(KgrDictionary {
                        { "tag", current != nullptr },
                        { "content", this->document->conv.ReverseConvert(buffer) }
                    });
                }
                lineIdx++;
                result.emplace_back(std::move(fragments));
            });
            return result;
        }

        struct DocumentContent {
            DocumentContent(SlokedEditorDocument &document, const SlokedCharPreset &charPreset)
                : text(document.GetText()), encoding(document.GetEncoding()), conv(SlokedLocale::SystemEncoding(), document.GetEncoding()),
                  transactionListeners(document.GetTransactionListeners()), tags(document.GetTagger()),
                  taggersUpdated{false} {

                this->updateListener = std::make_shared<DocumentUpdateListener>();
                this->transactionListeners.AddListener(this->updateListener);
                this->unsubscribeTaggers = document.GetTagger().OnChange([this](const auto &) {
                    this->taggersUpdated = true;
                });
            }

            ~DocumentContent() {
                if (this->unsubscribeTaggers) {
                    this->unsubscribeTaggers();
                }
            }
                
            TextBlock &text;
            const Encoding &encoding;
            EncodingConverter conv;
            SlokedTransactionListenerManager &transactionListeners;
            SlokedTextTagger<SlokedEditorDocument::TagType> &tags;
            std::shared_ptr<DocumentUpdateListener> updateListener;
            SlokedTextTagger<SlokedEditorDocument::TagType>::Unbind unsubscribeTaggers;
            std::atomic<bool> taggersUpdated;
        };

        SlokedEditorDocumentSet &documents;
        const SlokedCharPreset &charPreset;
        SlokedEditorDocumentSet::Document handle;
        std::unique_ptr<DocumentContent> document;
        SlokedOrderedCache<TextPosition::Line, KgrValue> cache;
    };


    SlokedTextRenderService::SlokedTextRenderService(SlokedEditorDocumentSet &documents, const SlokedCharPreset &charPreset, KgrContextManager<KgrLocalContext> &contextManager)
        : documents(documents), charPreset(charPreset), contextManager(contextManager) {}

    void SlokedTextRenderService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedTextRenderContext>(std::move(pipe), documents, this->charPreset);
        this->contextManager.Attach(std::move(ctx));
    }

    SlokedTextRenderClient::SlokedTextRenderClient(std::unique_ptr<KgrPipe> pipe, SlokedEditorDocumentSet::DocumentId docId)
        : client(std::move(pipe)) {
        auto rsp = this->client.Invoke("attach", KgrDictionary {
            { "document", static_cast<int64_t>(docId) }
        });
    }

    std::optional<TextPosition> SlokedTextRenderClient::RealPosition(TextPosition src) {
        auto rsp = this->client.Invoke("realPosition", KgrDictionary {
            { "line", static_cast<int64_t>(src.line) },
            { "column", static_cast<int64_t>(src.column) }
        });
        auto renderRes = rsp.Get();
        if (!renderRes.HasResult()) {
            return {};
        }
        return TextPosition {
            static_cast<TextPosition::Line>(renderRes.GetResult().AsDictionary()["line"].AsInt()),
            static_cast<TextPosition::Column>(renderRes.GetResult().AsDictionary()["column"].AsInt())
        };
    }

    std::optional<KgrValue> SlokedTextRenderClient::Render(TextPosition::Line line, TextPosition::Line height) {
        auto rsp = this->client.Invoke("render", KgrDictionary {
            { "height", static_cast<int64_t>(height) },
            { "line", static_cast<int64_t>(line) }
        });
        auto renderRes = rsp.Get();
        if (!renderRes.HasResult()) {
            return {};
        }
        return std::move(renderRes.GetResult());
    }
}