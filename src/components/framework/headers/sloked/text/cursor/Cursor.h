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

#ifndef SLOKED_TEXT_CURSOR_CURSOR_H_
#define SLOKED_TEXT_CURSOR_CURSOR_H_

#include <string>
#include <vector>

#include "sloked/Base.h"
#include "sloked/core/Position.h"

namespace sloked {

    class SlokedCursor {
     public:
        using Line = TextPosition::Line;
        using Column = TextPosition::Column;

        virtual ~SlokedCursor() = default;

        virtual Line GetLine() const = 0;
        virtual Column GetColumn() const = 0;

        virtual void SetPosition(Line, Column) = 0;
        virtual void MoveUp(Line) = 0;
        virtual void MoveDown(Line) = 0;
        virtual void MoveForward(Column) = 0;
        virtual void MoveBackward(Column) = 0;

        virtual void Insert(std::string_view) = 0;
        virtual void NewLine(std::string_view) = 0;
        virtual void DeleteBackward() = 0;
        virtual void DeleteForward() = 0;
        virtual void ClearRegion(const TextPosition &);
        virtual void ClearRegion(const TextPosition &,
                                 const TextPosition &) = 0;
    };
}  // namespace sloked

#endif