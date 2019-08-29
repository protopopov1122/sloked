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

#include "sloked/screen/Splitter.h"
#include <cassert>

namespace sloked {

    Splitter::Constraints::Constraints(float dim, unsigned int min, unsigned int max)
        : dim(dim), min(min), max(max) {
        assert(dim >= 0.0f);
        assert(dim <= 1.0f);
        assert(min <= max || max == 0);
    }

    float Splitter::Constraints::GetDimensions() const {
        return this->dim;
    }

    unsigned int Splitter::Constraints::GetMinimum() const {
        return this->min;
    }

    unsigned int Splitter::Constraints::GetMaximum() const {
        return this->max;
    }
}