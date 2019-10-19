#include "sloked/services/DocumentSet.h"

namespace sloked {

    class SlokedDocumentSetContext : public SlokedServiceContext {
     public:
        SlokedDocumentSetContext(std::unique_ptr<KgrPipe> pipe, SlokedEditorDocumentSet &documents)
            : SlokedServiceContext(std::move(pipe)), documents(documents), document(documents.Empty()) {
            
            this->RegisterMethod("new", [this](const std::string &method, const KgrValue &params, Response &rsp) { this->New(method, params, rsp); });
            this->RegisterMethod("open", [this](const std::string &method, const KgrValue &params, Response &rsp) { this->Open(method, params, rsp); });
            this->RegisterMethod("openById", [this](const std::string &method, const KgrValue &params, Response &rsp) { this->OpenById(method, params, rsp); });
            this->RegisterMethod("save", [this](const std::string &method, const KgrValue &params, Response &rsp) { this->Save(method, params, rsp); });
            this->RegisterMethod("saveAs", [this](const std::string &method, const KgrValue &params, Response &rsp) { this->SaveAs(method, params, rsp); });
            this->RegisterMethod("close", [this](const std::string &method, const KgrValue &params, Response &rsp) { this->Close(method, params, rsp); });
            this->RegisterMethod("getId", [this](const std::string &method, const KgrValue &params, Response &rsp) { this->GetId(method, params, rsp); });
            this->RegisterMethod("getUpstream", [this](const std::string &method, const KgrValue &params, Response &rsp) { this->GetUpstream(method, params, rsp); });
        }

     protected:
        void New(const std::string &method, const KgrValue &params, Response &rsp) {
            const auto &prms = params.AsDictionary();
            const auto &encoding = prms["encoding"].AsString();
            const auto &newline = prms["newline"].AsString();
            const Encoding &enc = Encoding::Get(encoding);
            auto nl = NewLine::Create(newline, enc);
            this->document = this->documents.NewDocument(enc, std::move(nl));
            rsp.Result(static_cast<int64_t>(this->document.GetKey()));   
        }

        void Open(const std::string &method, const KgrValue &params, Response &rsp) {
            const auto &prms = params.AsDictionary();
            const auto &path = prms["path"].AsString();
            const auto &encoding = prms["encoding"].AsString();
            const auto &newline = prms["newline"].AsString();
            const Encoding &enc = Encoding::Get(encoding);
            auto nl = NewLine::Create(newline, enc);
            this->document = this->documents.OpenDocument(SlokedPath{path}, enc, std::move(nl));
            rsp.Result(static_cast<int64_t>(this->document.GetKey()));
        }

        void OpenById(const std::string &method, const KgrValue &params, Response &rsp) {
            auto id = static_cast<SlokedEditorDocumentSet::DocumentId>(params.AsInt());
            if (this->documents.HasDocument(id)) {
                this->document = this->documents.OpenDocument(id).value();
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void Save(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document.Exists() &&
                this->document.GetObject().HasUpstream()) {
                this->document.GetObject().Save();
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void SaveAs(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document.Exists()) {
                SlokedPath to{params.AsString()};
                this->documents.SaveAs(this->document.GetObject(), to);
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void Close(const std::string &method, const KgrValue &params, Response &rsp) {
            this->document.Release();
        }

        void GetId(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document.Exists()) {
                rsp.Result(static_cast<int64_t>(this->document.GetKey()));
            } else {
                rsp.Result({});
            }
        }

        void GetUpstream(const std::string &method, const KgrValue &params, Response &rsp) {
            if (this->document.Exists()) {
                auto upstream = this->document.GetObject().GetUpstream();
                if (upstream.has_value()) {
                    rsp.Result(upstream.value().get().ToString());
                } else {
                    rsp.Result({});
                }
            } else {
                rsp.Result({});
            }
        }

     private:
        SlokedEditorDocumentSet &documents;
        SlokedEditorDocumentSet::Document document;
    };

    SlokedDocumentSetService::SlokedDocumentSetService(SlokedEditorDocumentSet &documents, KgrContextManager<KgrLocalContext> &contextManager)
        : documents(documents), contextManager(contextManager) {}

    bool SlokedDocumentSetService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedDocumentSetContext>(std::move(pipe), this->documents);
        this->contextManager.Attach(std::move(ctx));
        return true;
    }

    SlokedDocumentSetClient::SlokedDocumentSetClient(std::unique_ptr<KgrPipe> pipe)
        : client(std::move(pipe)) {}

    std::optional<SlokedEditorDocumentSet::DocumentId> SlokedDocumentSetClient::New(const std::string &encoding, const std::string &newline) {
        auto rsp = this->client.Invoke("new", KgrDictionary {
            { "encoding", encoding },
            { "newline", newline }
        });
        auto res = rsp.Get();
        if (res.HasResult()) {
            return res.GetResult().AsInt();
        } else {
            return {};
        }
    }

    std::optional<SlokedEditorDocumentSet::DocumentId> SlokedDocumentSetClient::Open(const std::string &path, const std::string &encoding, const std::string &newline) {
        auto rsp = this->client.Invoke("open", KgrDictionary {
            { "path", path },
            { "encoding", encoding },
            { "newline", newline }
        });
        auto res = rsp.Get();
        if (res.HasResult()) {
            return res.GetResult().AsInt();
        } else {
            return {};
        }
    }

    bool SlokedDocumentSetClient::Open(SlokedEditorDocumentSet::DocumentId docId) {
        auto rsp = this->client.Invoke("openById", KgrDictionary {
            { "id", static_cast<int64_t>(docId) }
        });
        auto res = rsp.Get();
        if (res.HasResult()) {
            return res.GetResult().AsBoolean();
        } else {
            return false;
        }
    }

    bool SlokedDocumentSetClient::Save() {
        auto rsp = this->client.Invoke("save", {});
        auto res = rsp.Get();
        if (res.HasResult()) {
            return res.GetResult().AsBoolean();
        } else {
            return false;
        }
    }

    bool SlokedDocumentSetClient::Save(const std::string &path) {
        auto rsp = this->client.Invoke("saveAs", path);
        auto res = rsp.Get();
        if (res.HasResult()) {
            return res.GetResult().AsBoolean();
        } else {
            return false;
        }
    }

    void SlokedDocumentSetClient::Close() {
        this->client.Invoke("close", {});
    }

    std::optional<SlokedEditorDocumentSet::DocumentId> SlokedDocumentSetClient::GetId() {
        auto rsp = this->client.Invoke("getId", {});
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::Integer)) {
            return res.GetResult().AsInt();
        } else {
            return {};
        }
    }

    std::optional<std::string> SlokedDocumentSetClient::GetUpstream() {
        auto rsp = this->client.Invoke("getUpstream", {});
        auto res = rsp.Get();
        if (res.HasResult() && res.GetResult().Is(KgrValueType::String)) {
            return res.GetResult().AsString();
        } else {
            return {};
        }
    }
}