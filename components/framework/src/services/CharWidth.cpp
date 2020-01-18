#include "sloked/services/CharWidth.h"

namespace sloked {

    class SlokedCharWidthNotifyContext : public KgrLocalContext {
     public:
        SlokedCharWidthNotifyContext(std::unique_ptr<KgrPipe> pipe, const SlokedCharWidth &charWidth)
            : KgrLocalContext(std::move(pipe)), charWidth(charWidth), unbind{nullptr} {}

        ~SlokedCharWidthNotifyContext() {
            if (this->unbind != nullptr) {
                this->unbind();
            }
        }

        void Run() final {
            if (!this->pipe->Empty()) {
                this->Send();
                if (this->unbind == nullptr) {
                    this->unbind = this->charWidth.Listen([this](const SlokedCharWidth &) {
                        this->Send();
                    });
                }
                this->pipe->Drop(1);
            }
        }

     private:
        void Send() {
            this->pipe->Write(KgrDictionary {
                { "tabWidth", static_cast<int64_t>(charWidth.GetCharWidth(U'\t')) }
            });
        }
        const SlokedCharWidth &charWidth;
        std::function<void()> unbind;
    };

    SlokedCharWidthService::SlokedCharWidthService(const SlokedCharWidth &charWidth, KgrContextManager<KgrLocalContext> &contextManager)
        : charWidth(charWidth), contextManager(contextManager) {}
    
    void SlokedCharWidthService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedCharWidthNotifyContext>(std::move(pipe), this->charWidth);
        this->contextManager.Attach(std::move(ctx));
    }

    SlokedCharWidthClient::SlokedCharWidthClient(std::unique_ptr<KgrPipe> pipe, SlokedCharWidth &charWidth)
        : pipe(std::move(pipe)), charWidth(charWidth) {
        this->pipe->SetMessageListener([this] {
            while (!this->pipe->Empty()) {
                auto msg = this->pipe->Read();
                this->charWidth.SetTabWidth(msg.AsDictionary()["tabWidth"].AsInt());
            }
        });
        this->pipe->Write({});
    }

    void SlokedCharWidthClient::Close() {
        this->pipe->Close();
    }
}