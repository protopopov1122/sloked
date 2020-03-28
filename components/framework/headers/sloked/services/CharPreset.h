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

#ifndef SLOKED_SERVICES_CHARPRESET_H_
#define SLOKED_SERVICES_CHARPRESET_H_

#include "sloked/core/CharPreset.h"
#include "sloked/core/Closeable.h"
#include "sloked/kgr/ContextManager.h"
#include "sloked/kgr/Service.h"
#include "sloked/kgr/local/Context.h"

namespace sloked {

    class SlokedCharPresetService : public KgrService {
     public:
        SlokedCharPresetService(const SlokedCharPreset &,
                                KgrContextManager<KgrLocalContext> &);
        void Attach(std::unique_ptr<KgrPipe>) final;

     private:
        const SlokedCharPreset &charPreset;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedCharPresetClient : public SlokedCloseable {
     public:
        SlokedCharPresetClient(std::unique_ptr<KgrPipe>, SlokedCharPreset &);
        void Close() final;

     private:
        std::unique_ptr<KgrPipe> pipe;
        SlokedCharPreset &charPreset;
    };
}  // namespace sloked

#endif