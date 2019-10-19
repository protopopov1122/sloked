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
            : SlokedServiceContext(std::move(pipe)), documents(documents), handle(documents.Empty()), document(nullptr) {
            
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
        }

     protected:
        void Connect(const std::string &method, const KgrValue &params, Response &rsp) {
            auto doc = this->documents.OpenDocument(static_cast<SlokedEditorDocumentSet::DocumentId>(params.AsInt()));
            if (doc.has_value()) {
                this->handle = std::move(doc.value());
                this->document = std::make_unique<DocumentContent>(this->handle.GetObject());
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

    SlokedCursorClient::SlokedCursorClient(std::unique_ptr<KgrPipe> pipe)
        : client(std::move(pipe)) {}

    bool SlokedCursorClient::Connect(SlokedEditorDocumentSet::DocumentId docId) {
        auto rsp = this->client.Invoke("connect", static_cast<int64_t>(docId));
        auto res = rsp.Get();
        return res.HasResult() &&
            res.GetResult().AsBoolean();
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
}