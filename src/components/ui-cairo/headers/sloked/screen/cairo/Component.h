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

#ifndef SLOKED_SCREEN_CAIRO_COMPONENT_H_
#define SLOKED_SCREEN_CAIRO_COMPONENT_H_

#include <vector>

#include "sloked/screen/Keyboard.h"
#include "sloked/screen/cairo/Base.h"
#include "sloked/screen/graphics/Base.h"

namespace sloked {

    class SlokedCairoScreenComponent {
     public:
        using Dimensions = SlokedGraphicsDimensions;

        virtual ~SlokedCairoScreenComponent() = default;
        virtual bool CheckUpdates() = 0;
        virtual void ProcessInput(std::vector<SlokedKeyboardInput>) = 0;
        virtual void SetTarget(const Cairo::RefPtr<Cairo::Surface> &,
                               Dimensions) = 0;
        virtual Dimensions GetSize() const = 0;
    };
}  // namespace sloked

#endif