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

#include "sloked/core/CharWidth.h"

namespace sloked {

    SlokedCharWidth::SlokedCharWidth()
        : tab_width(4), tab(tab_width, U' ') {}

    std::size_t SlokedCharWidth::GetCharWidth(char32_t chr) const {
        if (chr != '\t') {
            return 1;
        } else {
            return this->tab_width;
        }
    }
    
    std::pair<std::size_t, std::size_t> SlokedCharWidth::GetRealPosition(std::string_view str, std::size_t idx, const Encoding &encoding) const {
        std::pair<std::size_t, std::size_t> res{0, 0};
        encoding.IterateCodepoints(str, [&](auto start, auto length, auto value) {
            res.first = res.second;
            res.second += GetCharWidth(value);
            return idx--;
        });
        return res;
    }

    std::string SlokedCharWidth::GetTab(const Encoding &encoding) const {
        return encoding.Encode(this->tab);
    }

    SlokedCharWidth::Unbind SlokedCharWidth::Listen(Listener listener) const {
        return this->events.Listen(std::move(listener));
    }

    void SlokedCharWidth::SetTabWidth(std::size_t width) {
        this->tab_width = width;
        this->tab = std::u32string(this->tab_width, U' ');
        this->events.Emit(*this);
    }
}