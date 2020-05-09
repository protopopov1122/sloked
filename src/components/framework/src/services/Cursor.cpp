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

#include "sloked/services/Cursor.h"

#include "sloked/core/Error.h"
#include "sloked/core/Locale.h"
#include "sloked/sched/CompoundTask.h"
#include "sloked/services/TextRender.h"
#include "sloked/text/cursor/TransactionCursor.h"
#include "sloked/text/cursor/TransactionJournal.h"
#include "sloked/text/fragment/Updater.h"

namespace sloked {

    class SlokedCursorContext : public SlokedServiceContext {
     public:
        SlokedCursorContext(std::unique_ptr<KgrPipe> pipe,
                            KgrServer::Connector renderConnector,
                            SlokedEditorDocumentSet &documents)
            : SlokedServiceContext(std::move(pipe)), documents(documents),
              handle(documents.Empty()),
              document(nullptr), sendResponses{true} {

            this->BindMethod("connect", &SlokedCursorContext::Connect);
            this->BindMethod("insert", &SlokedCursorContext::Insert);
            this->BindMethod("moveUp", &SlokedCursorContext::MoveUp);
            this->BindMethod("moveDown", &SlokedCursorContext::MoveDown);
            this->BindMethod("moveBackward",
                             &SlokedCursorContext::MoveBackward);
            this->BindMethod("moveForward", &SlokedCursorContext::MoveForward);
            this->BindMethod("newLine", &SlokedCursorContext::NewLine);
            this->BindMethod("deleteBackward",
                             &SlokedCursorContext::DeleteBackward);
            this->BindMethod("deleteForward",
                             &SlokedCursorContext::DeleteForward);
            this->BindMethod("undo", &SlokedCursorContext::Undo);
            this->BindMethod("redo", &SlokedCursorContext::Redo);
            this->BindMethod("getPosition", &SlokedCursorContext::GetPosition);
            this->BindMethod("moveTo", &SlokedCursorContext::SetPosition);
            this->BindMethod("clearRegion", &SlokedCursorContext::ClearRegion);
        }

     protected:
        void Connect(const std::string &method, const KgrValue &params,
                     Response &rsp) {
            auto docId = static_cast<SlokedEditorDocumentSet::DocumentId>(
                params.AsDictionary()["documentId"].AsInt());
            this->sendResponses = false;
            auto doc = this->documents.OpenDocument(docId);
            if (doc.has_value()) {
                if (params.AsDictionary().Has("sendResponses")) {
                    this->sendResponses =
                        params.AsDictionary()["sendResponses"].AsBoolean();
                }
                this->handle = std::move(doc.value());
                this->document = std::make_unique<DocumentContent>(
                    this->handle.GetObject(), docId);
                rsp.Result(true);
            } else {
                throw SlokedServiceError(
                    "CursorContext: Error while opening document");
            }
        }

        void Insert(const std::string &method, const KgrValue &params,
                    Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.Insert(
                    this->document->conv.Convert(params.AsString()));
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void MoveUp(const std::string &method, const KgrValue &params,
                    Response &rsp) {
            if (this->document != nullptr) {
                TextPosition::Line count{1};
                if (params.Is(KgrValueType::Object) &&
                    params.AsDictionary().Has("count")) {
                    count = params.AsDictionary()["count"].AsInt();
                }
                this->document->cursor.MoveUp(count);
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void MoveDown(const std::string &method, const KgrValue &params,
                      Response &rsp) {
            if (this->document != nullptr) {
                TextPosition::Line count{1};
                if (params.Is(KgrValueType::Object) &&
                    params.AsDictionary().Has("count")) {
                    count = params.AsDictionary()["count"].AsInt();
                }
                this->document->cursor.MoveDown(count);
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void MoveBackward(const std::string &method, const KgrValue &params,
                          Response &rsp) {
            if (this->document != nullptr) {
                TextPosition::Column count{1};
                if (params.Is(KgrValueType::Object) &&
                    params.AsDictionary().Has("count")) {
                    count = params.AsDictionary()["count"].AsInt();
                }
                this->document->cursor.MoveBackward(count);
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void MoveForward(const std::string &method, const KgrValue &params,
                         Response &rsp) {
            if (this->document != nullptr) {
                TextPosition::Column count{1};
                if (params.Is(KgrValueType::Object) &&
                    params.AsDictionary().Has("count")) {
                    count = params.AsDictionary()["count"].AsInt();
                }
                this->document->cursor.MoveForward(count);
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void NewLine(const std::string &method, const KgrValue &params,
                     Response &rsp) {
            if (this->document != nullptr) {
                std::string content{""};
                if (params.Is(KgrValueType::Object) &&
                    params.AsDictionary().Has("content")) {
                    content = this->document->conv.Convert(
                        params.AsDictionary()["content"].AsString());
                }
                this->document->cursor.NewLine(content);
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void DeleteBackward(const std::string &method, const KgrValue &params,
                            Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.DeleteBackward();
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void DeleteForward(const std::string &method, const KgrValue &params,
                           Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.DeleteForward();
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void Undo(const std::string &method, const KgrValue &params,
                  Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.Undo();
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void Redo(const std::string &method, const KgrValue &params,
                  Response &rsp) {
            if (this->document != nullptr) {
                this->document->cursor.Redo();
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void GetPosition(const std::string &method, const KgrValue &params,
                         Response &rsp) {
            if (this->document != nullptr) {
                rsp.Result(KgrDictionary{
                    {"line",
                     static_cast<int64_t>(this->document->cursor.GetLine())},
                    {"column", static_cast<int64_t>(
                                   this->document->cursor.GetColumn())}});
            } else {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void SetPosition(const std::string &method, const KgrValue &params,
                         Response &rsp) {
            if (this->document != nullptr) {
                const TextPosition position{
                    static_cast<TextPosition::Line>(
                        params.AsDictionary()["line"].AsInt()),
                    static_cast<TextPosition::Column>(
                        params.AsDictionary()["column"].AsInt())};
                this->document->cursor.SetPosition(position.line,
                                                   position.column);
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

        void ClearRegion(const std::string &method, const KgrValue &params,
                         Response &rsp) {
            if (this->document != nullptr) {
                const auto &dict = params.AsDictionary();
                const TextPosition to{
                    static_cast<TextPosition::Line>(
                        dict["to"].AsDictionary()["line"].AsInt()),
                    static_cast<TextPosition::Column>(
                        dict["to"].AsDictionary()["column"].AsInt())};
                if (dict.Has("from")) {
                    const TextPosition from{
                        static_cast<TextPosition::Line>(
                            dict["from"].AsDictionary()["line"].AsInt()),
                        static_cast<TextPosition::Column>(
                            dict["from"].AsDictionary()["column"].AsInt())};
                    this->document->cursor.ClearRegion(from, to);
                } else {
                    this->document->cursor.ClearRegion(to);
                }
                if (this->sendResponses) {
                    rsp.Result(true);
                }
            } else if (this->sendResponses) {
                throw SlokedServiceError(
                    "CursorContext: Document is not opened");
            }
        }

     private:
        struct DocumentContent {
            DocumentContent(SlokedEditorDocument &document,
                            SlokedEditorDocumentSet::DocumentId id)
                : documentId(id), text(document.GetText()),
                  conv(SlokedLocale::SystemEncoding(), document.GetEncoding()),
                  stream(document.NewStream()),
                  cursor(document.GetText(), document.GetEncoding(), *stream) {}

            SlokedEditorDocumentSet::DocumentId documentId;
            TextBlock &text;
            EncodingConverter conv;
            std::unique_ptr<SlokedTransactionStream> stream;
            TransactionCursor cursor;
        };

        SlokedEditorDocumentSet &documents;
        SlokedEditorDocumentSet::Document handle;
        std::unique_ptr<DocumentContent> document;
        bool sendResponses;
    };

    SlokedCursorService::SlokedCursorService(
        SlokedEditorDocumentSet &documents,
        KgrServer::Connector renderConnector,
        KgrContextManager<KgrLocalContext> &contextManager)
        : documents(documents), renderConnector(renderConnector),
          contextManager(contextManager) {}

    TaskResult<void> SlokedCursorService::Attach(
        std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        supplier.Wrap([&] {
            auto ctx = std::make_unique<SlokedCursorContext>(
                std::move(pipe), this->renderConnector, this->documents);
            this->contextManager.Attach(std::move(ctx));
        });
        return supplier.Result();
    }

    class SlokedCursorConnectionTask : public SlokedDeferredTask {
     public:
        SlokedCursorConnectionTask(
            std::shared_ptr<SlokedNetResponseBroker::Channel> rsp,
            std::function<void(bool)> callback)
            : callback(std::move(callback)) {
            rsp->Next().Notify([this](const auto &res) {
                std::unique_lock lock(this->mtx);
                this->result = res.Unwrap().HasResult() &&
                               res.Unwrap().GetResult().AsBoolean();
                if (this->ready) {
                    this->ready();
                }
            });
        }

        void Wait(std::function<void()> ready) final {
            std::unique_lock lock(this->mtx);
            if (this->result.has_value()) {
                ready();
            } else {
                this->ready = ready;
            }
        }

        void Run() final {
            if (this->result.has_value()) {
                this->callback(this->result.value());
            }
        }

     private:
        std::mutex mtx;
        std::function<void()> ready;
        std::function<void(bool)> callback;
        std::optional<bool> result;
    };

    SlokedCursorClient::SlokedCursorClient(std::unique_ptr<KgrPipe> pipe)
        : client(std::move(pipe)) {}

    std::unique_ptr<SlokedDeferredTask> SlokedCursorClient::Connect(
        SlokedEditorDocumentSet::DocumentId docId,
        std::function<void(bool)> callback) {
        return std::make_unique<SlokedCursorConnectionTask>(
            this->client.Invoke(
                "connect",
                KgrDictionary{{"documentId", static_cast<int64_t>(docId)}}),
            std::move(callback));
    }

    void SlokedCursorClient::Insert(const std::string &content) {
        this->client.Invoke("insert", content);
    }

    void SlokedCursorClient::MoveUp(TextPosition::Line count) {
        this->client.Invoke(
            "moveUp", KgrDictionary{{"count", static_cast<int64_t>(count)}});
    }

    void SlokedCursorClient::MoveDown(TextPosition::Line count) {
        this->client.Invoke(
            "moveDown", KgrDictionary{{"count", static_cast<int64_t>(count)}});
    }

    void SlokedCursorClient::MoveBackward(TextPosition::Column count) {
        this->client.Invoke(
            "moveBackward",
            KgrDictionary{{"count", static_cast<int64_t>(count)}});
    }

    void SlokedCursorClient::MoveForward(TextPosition::Column count) {
        this->client.Invoke(
            "moveForward",
            KgrDictionary{{"count", static_cast<int64_t>(count)}});
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

    void SlokedCursorClient::SetPosition(const TextPosition &target) {
        this->client.Invoke(
            "moveTo",
            KgrDictionary{{"line", static_cast<int64_t>(target.line)},
                          {"column", static_cast<int64_t>(target.column)}});
    }

    TaskResult<std::optional<TextPosition>> SlokedCursorClient::GetPosition() {
        return SlokedTaskTransformations::Transform(
            this->client.Invoke("getPosition", {})->Next(),
            [](const SlokedNetResponseBroker::Response &clientRes) {
                std::optional<TextPosition> result{};
                if (clientRes.HasResult()) {
                    const auto &cursor = clientRes.GetResult().AsDictionary();
                    result = TextPosition{
                        static_cast<TextPosition::Line>(cursor["line"].AsInt()),
                        static_cast<TextPosition::Column>(
                            cursor["column"].AsInt())};
                }
                return result;
            });
    }
}  // namespace sloked