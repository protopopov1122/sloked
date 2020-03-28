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

#include "sloked/screen/terminal/TerminalResize.h"

#include <csignal>

#ifdef SIGWINCH
namespace sloked {

    std::mutex SlokedTerminalResizeListener::mtx{};
    int64_t SlokedTerminalResizeListener::nextId{0};
    std::map<int64_t, std::function<void()>>
        SlokedTerminalResizeListener::listeners{};

    std::function<void()> SlokedTerminalResizeListener::Bind(
        std::function<void()> listener) {
        std::unique_lock lock(SlokedTerminalResizeListener::mtx);
        auto id = SlokedTerminalResizeListener::nextId++;
        if (SlokedTerminalResizeListener::listeners.empty()) {
            std::signal(SIGWINCH, &SlokedTerminalResizeListener::Trigger);
        }
        SlokedTerminalResizeListener::listeners.emplace(id,
                                                        std::move(listener));
        return [id] {
            std::unique_lock lock(SlokedTerminalResizeListener::mtx);
            if (SlokedTerminalResizeListener::listeners.count(id) != 0) {
                SlokedTerminalResizeListener::listeners.erase(id);
            }
        };
    }

    void SlokedTerminalResizeListener::Trigger(int) {
        std::unique_lock lock(SlokedTerminalResizeListener::mtx);
        for (auto it = SlokedTerminalResizeListener::listeners.begin();
             it != SlokedTerminalResizeListener::listeners.end();) {
            auto current = it++;
            lock.unlock();
            current->second();
            lock.lock();
        }
    }
}  // namespace sloked
#else
namespace sloked {

    std::function<void()> SlokedTerminalResizeListener::Bind(
        std::function<void()>) {
        return [] {};
    }
}  // namespace sloked
#endif