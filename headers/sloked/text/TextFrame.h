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

#ifndef SLOKED_TEXT_TEXTFRAME_H_
#define SLOKED_TEXT_TEXTFRAME_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/Position.h"
#include "sloked/core/CharWidth.h"
#include "sloked/text/TextBlock.h"
#include <vector>

namespace sloked {

    class TextFrameView : public TextBlockView {
     public:
        TextFrameView(const TextBlockView &, const Encoding &, const SlokedCharWidth &);

        std::size_t GetLastLine() const override;
        std::size_t GetTotalLength() const override;
        std::string_view GetLine(std::size_t) const override;
        bool Empty() const override;

        void Update(const TextPosition &, const TextPosition &);
        const TextPosition &GetOffset() const;
        const TextPosition &GetSize() const;

     protected:
        std::ostream &dump(std::ostream &) const override;

     private:
        void VisitLines(std::function<void(std::size_t, std::string_view)>) const;
        std::string PreprocessLine(std::string_view) const;

        const TextBlockView &text;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        TextPosition offset;
        TextPosition size;
        mutable std::vector<std::string> buffer;
    };
}

#endif