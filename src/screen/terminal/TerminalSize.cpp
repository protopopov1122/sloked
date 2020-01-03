/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

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

#include "sloked/screen/terminal/TerminalSize.h"
#include "sloked/screen/terminal/TerminalResize.h"

namespace sloked {

    SlokedTerminalSize::SlokedTerminalSize(SlokedTerminal &terminal)
        : terminal(terminal), nextId{0} {
        this->unsubscribe = SlokedTerminalResizeListener::Bind([this] {
            this->Trigger();
        });
    }

    SlokedTerminalSize::~SlokedTerminalSize() {
        if (this->unsubscribe) {
            this->unsubscribe();
        }
    }

    TextPosition SlokedTerminalSize::GetSize() const {
        return {
            this->terminal.GetHeight(),
            this->terminal.GetWidth()
        };
    }

    std::function<void()> SlokedTerminalSize::Listen(Listener listener) {
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

    void SlokedTerminalSize::Trigger() {
        this->terminal.UpdateDimensions();
        auto size = this->GetSize();
        std::unique_lock lock(this->mtx);
        for (auto it = this->listeners.begin(); it != this->listeners.end();) {
            auto current = it++;
            lock.unlock();
            current->second(size);
            lock.lock();
        }
    }
}