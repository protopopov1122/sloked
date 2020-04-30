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

#ifndef SLOKED_CORE_CHARPRESET_H_
#define SLOKED_CORE_CHARPRESET_H_

#include <cinttypes>

#include "sloked/core/Encoding.h"
#include "sloked/core/Event.h"

namespace sloked {

    class SlokedCharPreset {
     public:
        using Listener = SlokedEventEmitter<const SlokedCharPreset &>::Listener;
        using Unbind = SlokedEventEmitter<const SlokedCharPreset &>::Unbind;

        SlokedCharPreset();
        std::size_t GetCharWidth(char32_t, std::size_t) const;
        std::pair<std::size_t, std::size_t> GetRealPosition(
            std::string_view, std::size_t, const Encoding &) const;
        std::string GetTab(const Encoding &, std::size_t) const;
        Unbind Listen(Listener) const;

        void SetTabWidth(std::size_t);

     private:
        std::size_t tab_width;
        std::u32string tab;
        mutable SlokedEventEmitter<const SlokedCharPreset &> events;
    };
}  // namespace sloked

#endif