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

#ifndef SLOKED_KGR_LOCAL_CTX_MANAGER_RUNNABLECONTEXTMANAGER_H_
#define SLOKED_KGR_LOCAL_CTX_MANAGER_RUNNABLECONTEXTMANAGER_H_

#include "sloked/kgr/ContextManager.h"
#include "sloked/core/Runnable.h"
#include <mutex>
#include <list>

namespace sloked {

    template <typename T>
    class KgrRunnableContextManager : public KgrContextManager<T>, public SlokedRunnable {
     public:
        void Attach(std::unique_ptr<T> ctx) override {
            std::unique_lock<std::mutex> lock(this->contexts_mtx);
            this->contexts.push_back(std::move(ctx));
        }

        void Run() override {
            if (this->contexts.empty()) {
                return;
            }
            for (auto it = this->contexts.begin();;) {
                if ((*it)->GetState() == KgrServiceContext::State::Active) {
                    (*it)->Run();
                }
                
                std::unique_lock<std::mutex> lock(this->contexts_mtx);
                auto current_it = it;
                ++it;
                auto state = (*current_it)->GetState();
                if (state == KgrServiceContext::State::Finished ||
                    state == KgrServiceContext::State::Destroyed) {
                    this->contexts.erase(current_it);
                }
                if (it == this->contexts.end()) {
                    break;
                }
            }
        }
    
     private:
        std::list<std::unique_ptr<T>> contexts;
        std::mutex contexts_mtx;
    };
}

#endif