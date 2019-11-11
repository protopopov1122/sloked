#include "sloked/services/DocumentNotify.h"

namespace sloked {
    class SlokedDocumentNotifyListener : public SlokedTransactionStreamListener {
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
        SlokedDocumentNotifyContext(std::unique_ptr<KgrPipe> pipe, SlokedEditorDocumentSet &documents)
            : KgrLocalContext(std::move(pipe)), documents(documents) {
            this->notifier = std::make_shared<SlokedDocumentNotifyListener>([this] {
                this->pipe->Write({});
            });
        }

        virtual ~SlokedDocumentNotifyContext() {
            if (this->document.has_value()) {
                this->document.value().GetObject().GetTransactionListeners().RemoveListener(*this->notifier);
            }
        }

        void Run() override {
            if (!this->pipe->Empty()) {
                auto msg = this->pipe->Read();
                auto docId = msg.AsInt();
                if (this->document.has_value()) {
                    this->document.value().GetObject().GetTransactionListeners().RemoveListener(*this->notifier);
                }
                this->document = this->documents.OpenDocument(docId);
                if (this->document.has_value()) {
                    this->document.value().GetObject().GetTransactionListeners().AddListener(this->notifier);
                }
            }
        }

     private:
        SlokedEditorDocumentSet &documents;
        std::optional<SlokedEditorDocumentSet::Document> document;
        std::shared_ptr<SlokedDocumentNotifyListener> notifier;
    };

    SlokedDocumentNotifyService::SlokedDocumentNotifyService(SlokedEditorDocumentSet &documents, KgrContextManager<KgrLocalContext> &ctxManager)
        : documents(documents), contextManager(ctxManager) {}

    bool SlokedDocumentNotifyService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedDocumentNotifyContext>(std::move(pipe), this->documents);
        this->contextManager.Attach(std::move(ctx));
        return true;
    }

    SlokedDocumentNotifyClient::SlokedDocumentNotifyClient(std::unique_ptr<KgrPipe> pipe, SlokedEditorDocumentSet::DocumentId docId)
        : pipe(std::move(pipe)) {
        this->pipe->Write(static_cast<int64_t>(docId));
    }

    void SlokedDocumentNotifyClient::OnUpdate(std::function<void()> listener) {
        this->pipe->SetMessageListener([this, listener] {
            this->pipe->DropAll();
            listener();
        });
    }
}