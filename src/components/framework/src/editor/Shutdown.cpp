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

#include "sloked/editor/Shutdown.h"

namespace sloked {

    SlokedDefaultEditorShutdown::SlokedDefaultEditorShutdown()
        : shutdown_requested{false} {}

    bool SlokedDefaultEditorShutdown::RequestShutdown() {
        std::unique_lock lock(this->mtx);
        this->shutdown_requested = true;
        this->cv.notify_all();
        return true;
    }

    void SlokedDefaultEditorShutdown::WaitForShutdown() {
        std::unique_lock lock(this->mtx);
        this->cv.wait(lock, [this] {
            return this->shutdown_requested;
        });
    }
}