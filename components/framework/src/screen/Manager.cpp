#include "sloked/screen/Manager.h"
#include <algorithm>

namespace sloked {

    SlokedScreenManager::SlokedScreenManager()
        : running{false} {}

    bool SlokedScreenManager::IsRunning() const {
        return this->running.load();
    }

    void SlokedScreenManager::Start(Duration timeout) {
        if (!this->running.exchange(true)) {
            this->worker = std::thread([this, timeout] {
                this->Run(timeout);
            });
        }
    }

    void SlokedScreenManager::Stop() {
        if (this->running.exchange(false) && this->worker.joinable()) {
            this->worker.join();
        }
    }

    void SlokedScreenManager::Repaint() {
        this->cv.notify_one();
    }

    void SlokedScreenManager::Attach(Renderable &renderable) {
        std::unique_lock lock(this->mtx);
        this->renderables.emplace_back(std::ref(renderable));
    }

    void SlokedScreenManager::Detach(Renderable &renderable) {
        std::unique_lock lock(this->mtx);
        this->renderables.erase(std::remove_if(this->renderables.begin(), this->renderables.end(), [&](auto other) {
            return std::addressof(renderable) == std::addressof(other.get());
        }), this->renderables.end());
    }

    void SlokedScreenManager::Run(Duration timeout) {
        std::unique_lock lock(this->mtx);
        while (this->running.load()) {
            for (auto renderable : this->renderables) {
                renderable.get().Render();
            }
            this->cv.wait_for(lock, timeout);
        }
    }
}