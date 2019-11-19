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
#include "sloked/services/TextRender.h"

namespace sloked {

    class SlokedCursorContext : public SlokedServiceContext {
     public:
        SlokedCursorContext(std::unique_ptr<KgrPipe> pipe, KgrServer::Connector renderConnector,
            SlokedEditorDocumentSet &documents)
            : SlokedServiceContext(std::move(pipe)), documents(documents), renderConnector(std::move(renderConnector)), handle(documents.Empty()), document(nullptr), renderClient(nullptr) {
            
            this->BindMethod("connect", &SlokedCursorContext::Connect);
            this->BindMethod("insert", &SlokedCursorContext::Insert);
            this->BindMethod("moveUp", &SlokedCursorContext::MoveUp);
            this->BindMethod("moveDown", &SlokedCursorContext::MoveDown);
            this->BindMethod("moveBackward", &SlokedCursorContext::MoveBackward);
            this->BindMethod("moveForward", &SlokedCursorContext::MoveForward);
            this->BindMethod("newLine", &SlokedCursorContext::NewLine);
            this->BindMethod("deleteBackward", &SlokedCursorContext::DeleteBackward);
            this->BindMethod("deleteForward", &SlokedCursorContext::DeleteForward);
            this->BindMethod("undo", &SlokedCursorContext::Undo);
            this->BindMethod("redo", &SlokedCursorContext::Redo);
            this->BindMethod("getPosition", &SlokedCursorContext::GetPosition);
            this->BindMethod("render", &SlokedCursorContext::Render);
        }

     protected:
        void Connect(const std::string &method, const KgrValue &params, Response &rsp) {
            auto docId = static_cast<SlokedEditorDocumentSet::DocumentId>(params.AsInt());
            auto doc = this->documents.OpenDocument(docId);
            if (doc.has_value()) {
                this->handle = std::move(doc.value());
                this->document = std::make_unique<DocumentContent>(this->handle.GetObject(), docId);
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void Insert(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.Insert(this->document->conv.Convert(params.AsString()));
            }
        }

        void MoveUp(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.MoveUp(1);
            }
        }

        void MoveDown(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.MoveDown(1);
            }
        }

        void MoveBackward(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.MoveBackward(1);
            }
        }

        void MoveForward(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.MoveForward(1);
            }
        }

        void NewLine(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.NewLine("");
            }
        }

        void DeleteBackward(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.DeleteBackward();
            }
        }

        void DeleteForward(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.DeleteForward();
            }
        }

        void Undo(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.Undo();
            }
        }

        void Redo(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.Redo();
            }
        }

        void GetPosition(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                rsp.Result(KgrDictionary {
                    { "line", static_cast<int64_t>(this->document->cursor.GetLine()) },
                    { "column", static_cast<int64_t>(this->document->cursor.GetColumn()) }
                });
            } else {
                rsp.Result({});
            }
        }

        void Render(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document != nullptr) {
                TextPosition dim {
                    static_cast<TextPosition::Line>(params.AsDictionary()["height"].AsInt()),
                    static_cast<TextPosition::Column>(params.AsDictionary()["width"].AsInt())
                };
                if (this->renderClient == nullptr) {
                    this->renderClient = std::make_shared<SlokedServiceClient>(this->renderConnector());
                    renderClient->Invoke("attach", static_cast<int64_t>(this->document->documentId));
                }
                auto renderResponse = std::make_shared<SlokedServiceClient::ResponseHandle>(this->renderClient->Invoke("render", KgrDictionary {
                    { "height", static_cast<int64_t>(dim.line) },
                    { "width", static_cast<int64_t>(dim.column) },
                    { "line", static_cast<int64_t>(this->document->cursor.GetLine()) },
                    { "column", static_cast<int64_t>(this->document->cursor.GetColumn()) }
                }));

                this->Defer(std::make_unique<SlokedDynamicAsyncTask>([this, renderClient = this->renderClient, response = std::move(rsp), renderResponse](Callback cb) mutable {
                    renderResponse->Notify(cb);
                    return [this, renderClient, response = std::move(response), renderResponse]() mutable {
                        auto res = renderResponse->GetOptional();
                        if (res.has_value() && res.value().HasResult()) {
                            response.Result(KgrDictionary {
                                { "render", res.value().GetResult() },
                                {
                                    "cursor",
                                    KgrDictionary {
                                        { "line", static_cast<int64_t>(this->document->cursor.GetLine()) },
                                        { "column", static_cast<int64_t>(this->document->cursor.GetColumn()) }
                                    }
                                }
                            });
                        } else {
                            response.Result({});
                        }
                        return false;
                    };
                }));
            } else {
                rsp.Result({});
            }
        }

     private:
        struct DocumentContent {
            DocumentContent(SlokedEditorDocument &document, SlokedEditorDocumentSet::DocumentId id)
                : documentId(id), text(document.GetText()), conv(SlokedLocale::SystemEncoding(), document.GetEncoding()),
                  stream(document.NewStream()), cursor(document.GetText(), document.GetEncoding(), *stream) {}
                
            SlokedEditorDocumentSet::DocumentId documentId;
            TextBlock &text;
            EncodingConverter conv;
            std::unique_ptr<SlokedTransactionStream> stream;
            TransactionCursor cursor;
        };

        SlokedEditorDocumentSet &documents;
        KgrServer::Connector renderConnector;
        SlokedEditorDocumentSet::Document handle;
        std::unique_ptr<DocumentContent> document;
        std::shared_ptr<SlokedServiceClient> renderClient;
    };


    SlokedCursorService::SlokedCursorService(SlokedEditorDocumentSet &documents, KgrServer::Connector renderConnector, KgrContextManager<KgrLocalContext> &contextManager)
        : documents(documents), renderConnector(renderConnector), contextManager(contextManager) {}

    bool SlokedCursorService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedCursorContext>(std::move(pipe), this->renderConnector, this->documents);
        this->contextManager.Attach(std::move(ctx));
        return true;
    }

    SlokedCursorClient::SlokedCursorClient(std::unique_ptr<KgrPipe> pipe, SlokedSchedulerThread &sched)
        : client(std::move(pipe)), sched(sched) {}

    void SlokedCursorClient::Connect(SlokedEditorDocumentSet::DocumentId docId, std::function<void(bool)> callback) {
        auto waiter = std::make_shared<SlokedServiceClient::ResponseWaiter>(this->client.Invoke("connect", static_cast<int64_t>(docId)), this->sched);
        waiter->Wait([waiter = std::move(waiter), callback = std::move(callback)](auto &res) {
            auto result = res.HasResult() && res.GetResult().AsBoolean();
            callback(result);
        });
    }

    void SlokedCursorClient::Insert(const std::string &content) {
        this->client.Invoke("insert", content);
    }
    
    void SlokedCursorClient::MoveUp() {
        this->client.Invoke("moveUp", {});
    }
    
    void SlokedCursorClient::MoveDown() {
        this->client.Invoke("moveDown", {});
    }
    
    void SlokedCursorClient::MoveBackward() {
        this->client.Invoke("moveBackward", {});
    }
    
    void SlokedCursorClient::MoveForward() {
        this->client.Invoke("moveForward", {});
    }
    
    void SlokedCursorClient::NewLine() {
        this->client.Invoke("newLine", {});
    }
    
    void SlokedCursorClient::DeleteBackward() {
        this->client.Invoke("deleteBackward", {});
    }
    
    void SlokedCursorClient::DeleteForward() {
        this->client.Invoke("deleteForward", {});
    }
    
    void SlokedCursorClient::Undo() {
        this->client.Invoke("undo", {});
    }
    
    void SlokedCursorClient::Redo() {
        this->client.Invoke("redo", {});
    }

    std::optional<TextPosition> SlokedCursorClient::GetPosition() {
        auto rsp = this->client.Invoke("getPosition", {});
        auto clientRes = rsp.Get();
        if (!clientRes.HasResult()) {
            return {};
        } else {
            const auto &cursor = clientRes.GetResult().AsDictionary();
            return TextPosition {
                static_cast<TextPosition::Line>(cursor["line"].AsInt()),
                static_cast<TextPosition::Column>(cursor["column"].AsInt())
            };
        }
    }

    std::optional<std::pair<KgrValue, TextPosition>> SlokedCursorClient::Render(const TextPosition &dim) {
        auto rsp = this->client.Invoke("render", KgrDictionary {
            { "height", static_cast<int64_t>(dim.line) },
            { "width", static_cast<int64_t>(dim.column) }
        });
        auto clientRes = rsp.Get();
        if (clientRes.HasResult()) {
            const auto &cursor = clientRes.GetResult().AsDictionary()["cursor"].AsDictionary();
            TextPosition pos {
                static_cast<TextPosition::Line>(cursor["line"].AsInt()),
                static_cast<TextPosition::Column>(cursor["column"].AsInt())
            };
            return std::make_pair(clientRes.GetResult().AsDictionary()["render"], pos);
        } else {
            return {};
        }
    }
}