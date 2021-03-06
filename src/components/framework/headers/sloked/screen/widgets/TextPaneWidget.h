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

#ifndef SLOKED_SCREEN_WIDGETS_TEXTPANEWIDGET_H_
#define SLOKED_SCREEN_WIDGETS_TEXTPANEWIDGET_H_

#include "sloked/screen/Component.h"
#include "sloked/screen/widgets/TextPane.h"

namespace sloked {

    class SlokedTextPaneWidget {
     public:
        virtual ~SlokedTextPaneWidget() = default;
        virtual bool ProcessInput(const SlokedKeyboardInput &) = 0;
        virtual TaskResult<void> RenderSurface(
            SlokedGraphicsPoint::Coordinate, TextPosition::Line,
            const SlokedFontProperties &) = 0;
        virtual void ShowSurface(SlokedTextPane &) = 0;
        virtual void OnUpdate(std::function<void()>) = 0;
    };
}  // namespace sloked

#endif