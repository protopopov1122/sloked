#ifndef SLOKED_CORE_LISTENER_H_
#define SLOKED_CORE_LISTENER_H_

#include "sloked/Base.h"
#include <memory>
#include <vector>
#include <algorithm>

namespace sloked {

    template <typename T, typename E, typename P>
    class SlokedListenerManager : public virtual P {
     public:
        virtual ~SlokedListenerManager() = default;
        
        void AddListener(std::shared_ptr<T> listener) override {
            this->listeners.push_back(listener);
        }

        void RemoveListener(const T &listener) override {
            std::remove_if(this->listeners.begin(), this->listeners.end(), [&](const auto &l) {
                return l.get() == &listener;
            });
        }

        void ClearListeners() override {
            this->listeners.clear();
        }

     protected:
        using ListenerMethod = void (T::*)(const E &);
        void TriggerListeners(ListenerMethod method, const E &event) {
            for (const auto &listener : this->listeners) {
                (*listener.*method)(event);
            }
        }

     private:
        std::vector<std::shared_ptr<T>> listeners;
    };
}

#endif