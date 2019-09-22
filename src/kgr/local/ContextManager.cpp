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

    KgrLocalContextManager::Unbinder::Unbinder(KgrLocalContextManager &ctx_man, KgrLocalContext &ctx)
        : manager(ctx_man), context(ctx) {}
    
    KgrLocalContextManager::Unbinder::~Unbinder() {
        std::remove_if(this->manager.contexts.begin(), this->manager.contexts.end(), [&](const auto &ctx) {
            return std::addressof(ctx.get()) == std::addressof(this->context);
        });
    }

    std::unique_ptr<KgrLocalContextManager::ContextHandle> KgrLocalContextManager::Bind(KgrLocalContext &ctx) {
        this->contexts.push_back(std::ref(ctx));
        return std::make_unique<Unbinder>(*this, ctx);
    }
    
    void KgrLocalContextManager::Run() {
        for (const auto &ctx : this->contexts) {
            ctx.get().Run();
        }
    }
}