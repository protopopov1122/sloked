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

#include "sloked/screen/Point.h"

#include <limits>

namespace sloked {

    const SlokedGraphicsPoint SlokedGraphicsPoint::Min{0, 0};
    const SlokedGraphicsPoint SlokedGraphicsPoint::Max{
        std::numeric_limits<SlokedGraphicsPoint::Coordinate>::max(),
        std::numeric_limits<SlokedGraphicsPoint::Coordinate>::max()};

    bool SlokedGraphicsPoint::operator==(
        const SlokedGraphicsPoint &other) const {
        return this->x == other.x && this->y == other.y;
    }

    bool SlokedGraphicsPoint::operator!=(
        const SlokedGraphicsPoint &other) const {
        return !this->operator==(other);
    }

    SlokedGraphicsPoint SlokedGraphicsPoint::operator+(
        const SlokedGraphicsPoint &other) const {
        return SlokedGraphicsPoint{this->x + other.x, this->y + other.x};
    }

    SlokedGraphicsPoint SlokedGraphicsPoint::operator-(
        const SlokedGraphicsPoint &other) const {
        return SlokedGraphicsPoint{this->x - other.x, this->y - other.y};
    }
}  // namespace sloked