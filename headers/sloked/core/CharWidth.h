/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_CORE_CHARWIDTH_H_
#define SLOKED_CORE_CHARWIDTH_H_

#include "sloked/Base.h"
#include "sloked/core/Encoding.h"
#include <cinttypes>

namespace sloked {

    class SlokedCharWidth {
     public:
        SlokedCharWidth();
        std::size_t GetCharWidth(char32_t) const;
        std::pair<std::size_t, std::size_t> GetRealPosition(const std::string &, std::size_t, const Encoding &) const;
        std::string GetTab() const;
    
        void SetTabWidth(std::size_t);
     private:
        std::size_t tab_width;
    };
}

#endif