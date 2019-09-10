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

#include "sloked/core/Locale.h"
#include <locale>
#include <clocale>
#include <iostream>

namespace sloked {

    // TODO Proper system locale setup and detection

    void SlokedLocale::Setup() {
        constexpr auto Locale = "C";
        constexpr auto LocaleEnc = "C.UTF-8";
        std::setlocale(LC_ALL, LocaleEnc);
        std::locale::global(std::locale(Locale));
        std::cout.imbue(std::locale(Locale));
    }

    const Encoding &SlokedLocale::SystemEncoding() {
        return Encoding::Utf8;
    }
}