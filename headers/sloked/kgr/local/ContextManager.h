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

#ifndef SLOKED_KGR_LOCAL_CONTEXTMANAGER_H_
#define SLOKED_KGR_LOCAL_CONTEXTMANAGER_H_

#include "sloked/kgr/ContextManager.h"
#include "sloked/kgr/local/Context.h"

namespace sloked {

    class KgrLocalContextManager : public KgrContextManager<KgrLocalContext>, public SlokedRunnable {
     public:
        friend class Unbinder;
        class Unbinder : public ContextHandle {
         public:
            Unbinder(KgrLocalContextManager &, KgrLocalContext &);
            virtual ~Unbinder();

         private:
            KgrLocalContextManager &manager;
            KgrLocalContext &context;
        };

        std::unique_ptr<ContextHandle> Bind(KgrLocalContext &) override;
        void Run() override;
    
     private:
        std::vector<std::reference_wrapper<KgrLocalContext>> contexts;
    };
}

#endif