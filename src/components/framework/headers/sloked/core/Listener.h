/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_CORE_LISTENER_H_
#define SLOKED_CORE_LISTENER_H_

#include <algorithm>
#include <memory>
#include <vector>

#include "sloked/Base.h"

namespace sloked {

    template <typename T>
    class SlokedOptionalInheritance : public virtual T {
     public:
        using T::T;
    };

    template <>
    class SlokedOptionalInheritance<void> {};

    template <typename T, typename E, typename P>
    class SlokedListenerManager : public SlokedOptionalInheritance<P> {
     public:
        virtual ~SlokedListenerManager() = default;

        virtual void AddListener(std::shared_ptr<T> listener) {
            this->listeners.push_back(listener);
        }

        virtual void RemoveListener(const T &listener) {
            this->listeners.erase(
                std::remove_if(
                    this->listeners.begin(), this->listeners.end(),
                    [&](const auto &l) { return l.get() == &listener; }),
                this->listeners.end());
        }

        virtual void ClearListeners() {
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
}  // namespace sloked

#endif