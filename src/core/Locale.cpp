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

#include "sloked/core/Locale.h"
#include "sloked/core/Error.h"
#include <locale>
#include <clocale>
#include <iostream>

namespace sloked {

    std::reference_wrapper<const Encoding> SlokedLocale::encoding{Encoding::Utf8};

    void SlokedLocale::Setup() {
        std::locale userLocale("");
        auto locale = userLocale.name();
        auto separator = locale.find(".");
        if (locale == "*" || separator == locale.npos) {
            throw SlokedError("Locale: Unknown locale");
        }
        auto encoding = locale.substr(separator + 1);
        SlokedLocale::encoding = std::cref(Encoding::Get(encoding));
    }

    const Encoding &SlokedLocale::SystemEncoding() {
        return SlokedLocale::encoding.get();
    }

    std::unique_ptr<NewLine> SlokedLocale::SystemNewline(const Encoding &encoding) {
#if defined(SLOKED_PLATFORM_LINUX) || defined(SLOKED_PLATFORM_UNIX)
        return NewLine::LF(encoding);
#else
#error "Internal error: Unknown platform"
#endif
    }
}