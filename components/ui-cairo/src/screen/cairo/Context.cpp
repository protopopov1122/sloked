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

#include "sloked/screen/cairo/Context.h"

namespace sloked {

    SlokedCairoContext::SlokedCairoContext(const SlokedCairoSurface &surface)
        : context{cairo_create(surface.GetSurface())} {}

    SlokedCairoContext::SlokedCairoContext(SlokedCairoContext &&ctx)
        : context{ctx.context} {
        ctx.context = nullptr;
    }

    SlokedCairoContext::~SlokedCairoContext() {
        if (this->context) {
            cairo_destroy(this->context);
        }
    }

    SlokedCairoContext &SlokedCairoContext::operator=(SlokedCairoContext &&ctx) {
        if (this->context) {
            cairo_destroy(this->context);
        }
        this->context = ctx.context;
        ctx.context = nullptr;
        return *this;
    }

    cairo_t *SlokedCairoContext::GetContext() const {
        return this->context;
    }
}