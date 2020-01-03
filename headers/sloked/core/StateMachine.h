/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_CORE_STATEMACHINE_H_
#define SLOKED_CORE_STATEMACHINE_H_

#include "sloked/Base.h"
#include <functional>
#include <map>

namespace sloked {

    template <typename T>
    class SlokedStateMachine {
     public:
        virtual ~SlokedStateMachine() = default;

     protected:
        SlokedStateMachine(T initial)
            : state(initial) {}

        T GetState() const {
            return this->state;
        }

        bool Transition(T newstate) {
            this->state = newstate;
            return this->states.count(this->state) != 0;
        }

        bool RunTransition(T newstate) {
            this->state = newstate;
            if (this->states.count(this->state) != 0) {
                this->states.at(this->state)();
                return true;
            } else {
                return false;
            }
        }

        void BindState(T state, std::function<void()> callback) {
            this->states[state] = callback;
        }

        template <typename C>
        void BindState(T state, void (C::*method)()) {
            this->states[state] = [this, method] {
                (static_cast<C *>(this)->*method)();
            };
        }

        bool RunStep() const {
            if (this->states.count(this->state)) {
                this->states.at(this->state)();
                return true;
            } else {
                return false;
            }
        }

     private:
        T state;
        std::map<T, std::function<void()>> states;
    };
}

#endif