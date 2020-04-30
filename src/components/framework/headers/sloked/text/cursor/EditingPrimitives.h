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

#ifndef SLOKED_TEXT_CURSOR_EDITINGPRIMITIVES_H_
#define SLOKED_TEXT_CURSOR_EDITINGPRIMITIVES_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/Position.h"
#include "sloked/text/TextBlock.h"

namespace sloked {

    class SlokedEditingPrimitives {
     public:
        static TextPosition Insert(TextBlock &, const Encoding &,
                                   const TextPosition &, std::string_view);
        static TextPosition Newline(TextBlock &, const Encoding &,
                                    const TextPosition &, std::string_view);
        static TextPosition DeleteBackward(TextBlock &, const Encoding &,
                                           const TextPosition &);
        static TextPosition DeleteForward(TextBlock &, const Encoding &,
                                          const TextPosition &);
        static TextPosition ClearRegion(TextBlock &, const Encoding &,
                                        const TextPosition &,
                                        const TextPosition &);
        static std::size_t GetOffset(std::string_view, TextPosition::Column,
                                     const Encoding &);
        static std::vector<std::string> Read(const TextBlock &,
                                             const Encoding &,
                                             const TextPosition &,
                                             const TextPosition &);
    };
}  // namespace sloked

#endif