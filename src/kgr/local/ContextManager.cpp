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

#include "sloked/kgr/local/ContextManager.h"
#include <algorithm>

namespace sloked {

    void KgrLocalContextManager::Push(std::unique_ptr<KgrLocalContext> ctx) {
        std::unique_lock<std::mutex> lock(this->contexts_mtx);
        this->contexts.push_back(std::move(ctx));
    }
    
    void KgrLocalContextManager::Run() {
        if (this->contexts.empty()) {
            return;
        }
        for (auto it = this->contexts.begin();;) {
            if ((*it)->Alive()) {
                (*it)->Run();
            }
            
            std::unique_lock<std::mutex> lock(this->contexts_mtx);
            auto current_it = it;
            ++it;
            if ((*current_it)->Alive()) {
                this->contexts.erase(current_it);
            }
            if (it == this->contexts.end()) {
                break;
            }
        }
    }
}