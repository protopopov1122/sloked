#include "sloked/core/Closeable.h"

namespace sloked {

    SlokedCloseablePool::~SlokedCloseablePool() {
        this->Close();
    }

    void SlokedCloseablePool::Attach(SlokedCloseable &closeable) {
        this->closeables.push_back(std::ref(closeable));
    }

    void SlokedCloseablePool::Close() {
        for (auto it = this->closeables.rbegin(); it != this->closeables.rend(); ++it) {
            it->get().Close();
        }
        this->closeables.clear();
    }
}