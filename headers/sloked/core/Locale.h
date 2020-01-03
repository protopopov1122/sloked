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

#ifndef SLOKED_CORE_LOCALE_H_
#define SLOKED_CORE_LOCALE_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/NewLine.h"

namespace sloked {

    class SlokedLocale {
     public:
        static void Setup();
        static const Encoding &SystemEncoding();
        static std::unique_ptr<NewLine> SystemNewline(const Encoding &);

     private:
        static std::reference_wrapper<const Encoding> encoding;
    };
}

#endif