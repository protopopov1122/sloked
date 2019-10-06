#include "sloked/services/DocumentSet.h"

namespace sloked {

    class SlokedDocumentSetContext : public SlokedServiceContext {
     public:
        SlokedDocumentSetContext(std::unique_ptr<KgrPipe> pipe, SlokedEditorDocumentSet &documents)
            : SlokedServiceContext(std::move(pipe)), documents(documents), document(documents.Empty()) {}

     protected:
        void ProcessRequest(const KgrValue &req) override {
            const auto &prms = req.AsDictionary();
            auto cmd = static_cast<SlokedDocumentSetService::Command>(prms["command"].AsInt());
            switch (cmd) {
                case SlokedDocumentSetService::Command::Open: {
                    if (prms.Has("path")) {
                        const auto &path = prms["path"].AsString();
                        const auto &encoding = prms["encoding"].AsString();
                        const auto &newline = prms["newline"].AsString();
                        const Encoding &enc = Encoding::Get(encoding);
                        auto nl = NewLine::Create(newline, enc);
                        this->document = this->documents.OpenDocument(SlokedPath{path}, enc, std::move(nl));
                        this->SendResponse(static_cast<int64_t>(this->document.GetKey()));
                    } else {
                        auto id = static_cast<SlokedEditorDocumentSet::DocumentId>(prms["id"].AsInt());
                        if (this->documents.HasDocument(id)) {
                            this->document = this->documents.OpenDocument(id).value();
                            this->SendResponse(true);
                        } else {
                            this->SendResponse(false);
                        }
                    }
                } break;

                case SlokedDocumentSetService::Command::Close:
                    this->document.Release();
                    break;
                
                case SlokedDocumentSetService::Command::Get:
                    if (this->document.Exists()) {
                        this->SendResponse(static_cast<int64_t>(this->document.GetKey()));
                    } else {
                        this->SendResponse({});
                    }
                    break;
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
        : pipe(std::move(pipe)) {}

    SlokedEditorDocumentSet::DocumentId SlokedDocumentSetClient::Open(const std::string &path, const std::string &encoding, const std::string &newline) {
        this->pipe->Write(KgrDictionary {
            { "command", static_cast<int64_t>(SlokedDocumentSetService::Command::Open) },
            { "path", path },
            { "encoding", encoding },
            { "newline", newline }
        });
        return this->pipe->ReadWait().AsInt();
    }

    void SlokedDocumentSetClient::Close() {
        this->pipe->Write(KgrDictionary {
            { "command", static_cast<int64_t>(SlokedDocumentSetService::Command::Close) }
        });
    }

    std::optional<SlokedEditorDocumentSet::DocumentId> SlokedDocumentSetClient::Get() const {
        this->pipe->Write(KgrDictionary {
            { "command", static_cast<int64_t>(SlokedDocumentSetService::Command::Get) }
        });
        auto res = this->pipe->ReadWait();
        if (res.Is(KgrValueType::Integer)) {
            return res.AsInt();
        } else {
            return {};
        }
    }
}