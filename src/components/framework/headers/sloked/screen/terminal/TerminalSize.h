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

#ifndef SLOKED_SCREEN_TERMINAL_TERMINALSIZE_H_
#define SLOKED_SCREEN_TERMINAL_TERMINALSIZE_H_

#include <map>
#include <mutex>

#include "sloked/screen/Size.h"
#include "sloked/screen/terminal/Terminal.h"

namespace sloked {

    template <typename T>
    class SlokedTerminalSize : public SlokedScreenSize {
     public:
        SlokedTerminalSize(SlokedTerminal &terminal, const T &binder)
            : terminal(terminal), nextId{0} {
            this->unsubscribe = binder([this] { this->Trigger(); });
        }

        ~SlokedTerminalSize() {
            if (this->unsubscribe) {
                this->unsubscribe();
            }
        }

        TextPosition GetScreenSize() const {
            return {this->terminal.GetHeight(), this->terminal.GetWidth()};
        }

        std::function<void()> Listen(Listener listener) {
            std::unique_lock lock(this->mtx);
            auto id = this->nextId++;
            this->listeners.emplace(id, std::move(listener));
            return [this, id] {
                std::unique_lock lock(this->mtx);
                if (this->listeners.count(id) != 0) {
                    this->listeners.erase(id);
                }
            };
        }

     private:
        void Trigger() {
            this->terminal.UpdateDimensions();
            auto size = this->GetScreenSize();
            std::unique_lock lock(this->mtx);
            for (auto it = this->listeners.begin();
                 it != this->listeners.end();) {
                auto current = it++;
                lock.unlock();
                current->second(size);
                lock.lock();
            }
        }

        SlokedTerminal &terminal;
        std::mutex mtx;
        int64_t nextId;
        std::map<int64_t, Listener> listeners;
        std::function<void()> unsubscribe;
    };
}  // namespace sloked

#endif