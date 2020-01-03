/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#ifndef SLOKED_CORE_POSIX_TIME_H_
#define SLOKED_CORE_POSIX_TIME_H_

#include "sloked/Base.h"
#include <chrono>
#include <sys/time.h>

namespace sloked {

    template <typename T>
    void DurationToTimeval(const T &src, struct timeval &tv) {
        if (src  > std::chrono::seconds(1)) {
            tv.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(src).count();
            tv.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(src - std::chrono::seconds(tv.tv_sec)).count();
        } else {
            tv.tv_sec = 0;
            tv.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(src).count();
        }
    }
}

#endif