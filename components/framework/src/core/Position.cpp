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

#include "sloked/core/Position.h"

#include <limits>

namespace sloked {

    const TextPosition TextPosition::Min{0, 0};
    const TextPosition TextPosition::Max{
        std::numeric_limits<TextPosition::Line>::max(),
        std::numeric_limits<TextPosition::Column>::max()};

    bool TextPosition::operator<(const TextPosition &other) const {
        return this->line < other.line ||
               (this->line == other.line && this->column < other.column);
    }

    bool TextPosition::operator<=(const TextPosition &other) const {
        return this->operator<(other) || this->operator==(other);
    }

    bool TextPosition::operator>(const TextPosition &other) const {
        return !this->operator<=(other);
    }

    bool TextPosition::operator>=(const TextPosition &other) const {
        return !this->operator<(other);
    }

    bool TextPosition::operator==(const TextPosition &other) const {
        return this->line == other.line && this->column == other.column;
    }

    bool TextPosition::operator!=(const TextPosition &other) const {
        return !this->operator==(other);
    }

    TextPosition TextPosition::operator+(const TextPosition &other) const {
        return TextPosition{this->line + other.line,
                            this->column + other.column};
    }

    TextPosition TextPosition::operator-(const TextPosition &other) const {
        return TextPosition{this->line - other.line,
                            this->column - other.column};
    }

    bool TextPositionDelta::operator==(const TextPositionDelta &delta) const {
        return this->line == delta.line && this->column == delta.column;
    }
}  // namespace sloked