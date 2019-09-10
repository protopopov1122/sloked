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

#include "sloked/core/CharWidth.h"

namespace sloked {

    SlokedCharWidth::SlokedCharWidth()
        : tab_width(4) {}

    std::size_t SlokedCharWidth::GetCharWidth(char32_t chr) const {
        switch (chr) {
            case '\t':
                return this->tab_width;
            
            default:
                return 1;
        }
    }
    
    std::pair<std::size_t, std::size_t> SlokedCharWidth::GetRealPosition(const std::string &str, std::size_t idx, const Encoding &encoding) const {
        std::pair<std::size_t, std::size_t> res{0, 0};
        encoding.IterateCodepoints(str, [&](auto start, auto length, auto value) {
            res.first = res.second;
            res.second += GetCharWidth(value);
            return idx--;
        });
        return res;
    }

    std::string SlokedCharWidth::GetTab(const Encoding &encoding) const {
        return encoding.Encode(std::u32string(this->tab_width, U' '));
    }

    void SlokedCharWidth::SetTabWidth(std::size_t width) {
        this->tab_width = width;
    }
}