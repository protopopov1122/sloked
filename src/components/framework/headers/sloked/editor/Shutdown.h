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

#ifndef SLOKED_EDITOR_SHUTDOWN_H_
#define SLOKED_EDITOR_SHUTDOWN_H_

#include "sloked/Base.h"
#include <mutex>
#include <condition_variable>

namespace sloked {

    class SlokedEditorShutdown {
     public:
        virtual ~SlokedEditorShutdown() = default;
        virtual bool RequestShutdown() = 0;
        virtual void WaitForShutdown() = 0;
    };

    class SlokedDefaultEditorShutdown : public SlokedEditorShutdown {
     public:
        SlokedDefaultEditorShutdown();
        bool RequestShutdown() final;
        void WaitForShutdown() final;

     private:
        bool shutdown_requested;
        std::mutex mtx;
        std::condition_variable cv;
    };
}

#endif