/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

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