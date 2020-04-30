#include "sloked/services/CharPreset.h"

namespace sloked {

    class SlokedCharPresetNotifyContext : public KgrLocalContext {
     public:
        SlokedCharPresetNotifyContext(std::unique_ptr<KgrPipe> pipe,
                                      const SlokedCharPreset &charPreset)
            : KgrLocalContext(std::move(pipe)),
              charPreset(charPreset), unbind{nullptr} {}

        ~SlokedCharPresetNotifyContext() {
            if (this->unbind != nullptr) {
                this->unbind();
            }
        }

        void Run() final {
            if (!this->pipe->Empty()) {
                this->Send();
                if (this->unbind == nullptr) {
                    this->unbind = this->charPreset.Listen(
                        [this](const SlokedCharPreset &) { this->Send(); });
                }
                this->pipe->Drop(1);
            }
        }

     private:
        void Send() {
            this->pipe->Write(KgrDictionary{
                {"tabWidth",
                 static_cast<int64_t>(charPreset.GetCharWidth(U'\t', 0))}});
        }
        const SlokedCharPreset &charPreset;
        std::function<void()> unbind;
    };

    SlokedCharPresetService::SlokedCharPresetService(
        const SlokedCharPreset &charPreset,
        KgrContextManager<KgrLocalContext> &contextManager)
        : charPreset(charPreset), contextManager(contextManager) {}

    TaskResult<void> SlokedCharPresetService::Attach(
        std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        supplier.Wrap([&] {
            auto ctx = std::make_unique<SlokedCharPresetNotifyContext>(
                std::move(pipe), this->charPreset);
            this->contextManager.Attach(std::move(ctx));
        });
        return supplier.Result();
    }

    SlokedCharPresetClient::SlokedCharPresetClient(
        std::unique_ptr<KgrPipe> pipe, SlokedCharPreset &charPreset)
        : pipe(std::move(pipe)), charPreset(charPreset) {
        this->pipe->SetMessageListener([this] {
            while (!this->pipe->Empty()) {
                auto msg = this->pipe->Read();
                this->charPreset.SetTabWidth(
                    msg.AsDictionary()["tabWidth"].AsInt());
            }
        });
        this->pipe->Write({});
    }

    void SlokedCharPresetClient::Close() {
        this->pipe->Close();
    }
}  // namespace sloked