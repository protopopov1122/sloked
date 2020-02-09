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
            SlokedEditorDocumentSet &documents, const SlokedCharPreset &charPreset)
            : SlokedServiceContext(std::move(pipe)), documents(documents), charPreset(charPreset), handle(documents.Empty()), document(nullptr) {
                
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
            const std::string tab = this->charPreset.GetTab(this->document->encoding);

            KgrArray lines;
            auto lineIdx = lineNumber;
            this->document->text.Visit(lineNumber, std::min(height,  static_cast<TextPosition::Line>(this->document->text.GetLastLine() - lineNumber + 1)), [&](auto line) {
                std::optional<std::pair<std::string, std::optional<TaggedTextFragment<int>>>> back;
                KgrArray fragments;
                TextPosition::Column columnIdx{0};
                this->document->encoding.IterateCodepoints(line, [&](auto start, auto length, auto chr) {
                    std::string_view fragment = chr != U'\t' ? line.substr(start, length) : tab;
                    auto tag = this->document->tags.Get(TextPosition {
                        static_cast<TextPosition::Line>(lineIdx),
                        static_cast<TextPosition::Column>(columnIdx)
                    });
                    if (!back.has_value()) {
                        back = std::make_pair(fragment, tag);
                    } else if (back.value().second != tag) {
                        fragments.Append(KgrDictionary {
                            { "tag", back.value().second.has_value() },
                            { "content", KgrValue(this->document->conv.ReverseConvert(std::move(back.value().first))) }
                        });
                        back = std::make_pair(fragment, tag);
                    } else {
                        back.value().first.append(fragment);
                    }
                    columnIdx++;
                    return true;
                });
                if (back.has_value()) {
                    fragments.Append(KgrDictionary {
                        { "tag", back.value().second.has_value() },
                        { "content", KgrValue(this->document->conv.ReverseConvert(std::move(back.value().first))) }
                    });
                }
                lineIdx++;
                lines.Append(std::move(fragments));
            });
            rsp.Result(std::move(lines));
        }

     private:

        struct DocumentContent {
            DocumentContent(SlokedEditorDocument &document, const SlokedCharPreset &charPreset)
                : text(document.GetText()), encoding(document.GetEncoding()), conv(SlokedLocale::SystemEncoding(), document.GetEncoding()),
                  transactionListeners(document.GetTransactionListeners()), tags(document.GetTagger()),
                  frame(document.GetText(), document.GetEncoding(), charPreset), taggersUpdated{false} {

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
            TextFrameView frame;
            std::shared_ptr<DocumentUpdateListener> updateListener;
            SlokedTextTagger<SlokedEditorDocument::TagType>::Unbind unsubscribeTaggers;
            std::atomic<bool> taggersUpdated;
        };

        SlokedEditorDocumentSet &documents;
        const SlokedCharPreset &charPreset;
        SlokedEditorDocumentSet::Document handle;
        std::unique_ptr<DocumentContent> document;
        KgrArray rendered;
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