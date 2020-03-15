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

#ifndef SLOKED_SCREEN_CAIRO_COMPONENT_H_
#define SLOKED_SCREEN_CAIRO_COMPONENT_H_

#include "sloked/screen/cairo/Base.h"
#include "sloked/screen/Keyboard.h"
#include <vector>

namespace sloked {

    class SlokedCairoScreenComponent {
     public:
        using Dimensions = SlokedCairoScreenDimensions;

        virtual ~SlokedCairoScreenComponent() = default;
        virtual bool HasUpdates() const = 0;
        virtual void ProcessInput(std::vector<SlokedKeyboardInput>) = 0;
        virtual void Render(const Cairo::RefPtr<Cairo::Context> &) = 0;
        virtual void SetSize(Dimensions) = 0;
        virtual Dimensions GetSize() const = 0;
    };
}

#endif