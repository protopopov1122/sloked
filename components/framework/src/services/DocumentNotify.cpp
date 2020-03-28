/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sloked/services/DocumentNotify.h"

namespace sloked {
    class SlokedDocumentNotifyListener
        : public SlokedTransactionStreamListener {
     public:
        SlokedDocumentNotifyListener(std::function<void()> listener)
            : listener(std::move(listener)) {}

        void OnCommit(const SlokedCursorTransaction &) override {
            this->listener();
        }

        void OnRollback(const SlokedCursorTransaction &) override {
            this->listener();
        }

        void OnRevert(const SlokedCursorTransaction &) override {
            this->listener();
        }

     private:
        std::function<void()> listener;
    };

    class SlokedDocumentNotifyContext : public KgrLocalContext {
     public:
        SlokedDocumentNotifyContext(std::unique_ptr<KgrPipe> pipe,
                                    SlokedEditorDocumentSet &documents)
            : KgrLocalContext(std::move(pipe)),
              documents(documents), taggerUnsubscribe{nullptr} {
            this->notifier =
                std::make_shared<SlokedDocumentNotifyListener>([this] {
                    this->pipe->Write(KgrDictionary{{"source", "content"}});
                });
        }

        virtual ~SlokedDocumentNotifyContext() {
            if (this->document.has_value()) {
                this->document.value()
                    .GetObject()
                    .GetTransactionListeners()
                    .RemoveListener(*this->notifier);
            }
            if (this->taggerUnsubscribe) {
                this->taggerUnsubscribe();
            }
        }

        void Run() override {
            if (!this->pipe->Empty()) {
                auto msg = this->pipe->Read().AsDictionary();
                auto docId = msg["document"].AsInt();
                auto tagger = msg["tagger"].AsBoolean();
                if (this->document.has_value()) {
                    this->document.value()
                        .GetObject()
                        .GetTransactionListeners()
                        .RemoveListener(*this->notifier);
                }
                if (this->taggerUnsubscribe) {
                    this->taggerUnsubscribe();
                    this->taggerUnsubscribe = nullptr;
                }
                this->document = this->documents.OpenDocument(docId);
                if (this->document.has_value()) {
                    this->document.value()
                        .GetObject()
                        .GetTransactionListeners()
                        .AddListener(this->notifier);
                    if (tagger) {
                        this->taggerUnsubscribe =
                            this->document.value()
                                .GetObject()
                                .GetTagger()
                                .OnChange([this](const auto &pos) {
                                    this->pipe->Write(KgrDictionary{
                                        {"source", "tagger"},
                                        {"payload",
                                         KgrDictionary{
                                             {"start",
                                              KgrDictionary{
                                                  {"line", static_cast<int64_t>(
                                                               pos.start.line)},
                                                  {"column",
                                                   static_cast<int64_t>(
                                                       pos.start.column)}}},
                                             {"end",
                                              KgrDictionary{
                                                  {"line", static_cast<int64_t>(
                                                               pos.end.line)},
                                                  {"column",
                                                   static_cast<int64_t>(
                                                       pos.end.column)}}}}}});
                                });
                    }
                }
            }
        }

     private:
        SlokedEditorDocumentSet &documents;
        std::optional<SlokedEditorDocumentSet::Document> document;
        std::shared_ptr<SlokedDocumentNotifyListener> notifier;
        SlokedTextTagger<SlokedEditorDocument::TagType>::Unbind
            taggerUnsubscribe;
    };

    SlokedDocumentNotifyService::SlokedDocumentNotifyService(
        SlokedEditorDocumentSet &documents,
        KgrContextManager<KgrLocalContext> &ctxManager)
        : documents(documents), contextManager(ctxManager) {}

    void SlokedDocumentNotifyService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedDocumentNotifyContext>(
            std::move(pipe), this->documents);
        this->contextManager.Attach(std::move(ctx));
    }

    SlokedDocumentNotifyClient::SlokedDocumentNotifyClient(
        std::unique_ptr<KgrPipe> pipe,
        SlokedEditorDocumentSet::DocumentId docId, bool taggerNotifications)
        : pipe(std::move(pipe)) {
        this->pipe->Write(
            KgrDictionary{{"document", static_cast<int64_t>(docId)},
                          {"tagger", taggerNotifications}});
    }

    void SlokedDocumentNotifyClient::OnUpdate(
        std::function<void(const Notification &)> listener) {
        this->pipe->SetMessageListener([this, listener] {
            while (!this->pipe->Empty()) {
                auto msg = this->pipe->Read().AsDictionary();
                if (msg["source"].AsString() == "content") {
                    listener({});
                } else if (msg["source"].AsString() == "tagger") {
                    const auto &payload = msg["payload"].AsDictionary();
                    listener(TextPositionRange{
                        TextPosition{static_cast<TextPosition::Line>(
                                         payload["start"]
                                             .AsDictionary()["line"]
                                             .AsInt()),
                                     static_cast<TextPosition::Column>(
                                         payload["start"]
                                             .AsDictionary()["column"]
                                             .AsInt())},
                        TextPosition{
                            static_cast<TextPosition::Line>(
                                payload["end"].AsDictionary()["line"].AsInt()),
                            static_cast<TextPosition::Column>(
                                payload["end"]
                                    .AsDictionary()["column"]
                                    .AsInt())}});
                }
            }
        });
    }
}  // namespace sloked