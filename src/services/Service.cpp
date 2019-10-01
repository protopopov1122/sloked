#include "sloked/services/Service.h"

namespace sloked {


    void SlokedServiceContext::Run() {
        try {
            if (!this->pipe->Empty()) {
                auto msg = this->pipe->Read();
                if (this->requestHandlers.empty()) {
                    this->ProcessRequest(msg);
                } else {
                    this->requestHandlers.front()(msg);
                    this->requestHandlers.pop();
                }
            }
        } catch (const SlokedError &err) {
            this->HandleError(err);
        }
    }

    void SlokedServiceContext::SendResponse(KgrValue &&msg) {
        this->pipe->Write(std::forward<KgrValue>(msg));
    }

    void SlokedServiceContext::OnRequest(std::function<void(const KgrValue &)> callback) {
        this->requestHandlers.push(std::move(callback));
    }

    void SlokedServiceContext::HandleError(const SlokedError &err) {
        throw err;
    }
}