/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

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

        bool Update(const TextPosition &, const TextPosition &, bool = true);
        const TextPosition &GetOffset() const;
        const TextPosition &GetSize() const;
        void VisitSymbols(std::function<void(std::size_t, std::size_t, const std::vector<std::string_view> &)>) const;

     protected:
        std::ostream &dump(std::ostream &) const override;

     private:
        struct Line {
            std::vector<std::string_view> symbols;
            std::string tab;
            std::string content;
            std::size_t leftOffset;
        };

        void FullRender();
        void OptimizedRender(const TextPosition &);
        void VisitLines(std::function<void(std::size_t, std::string_view)>) const;
        std::unique_ptr<Line> CutLine(std::string_view) const;

        const TextBlockView &text;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        TextPosition offset;
        TextPosition size;
        std::vector<std::unique_ptr<Line>> buffer;
    };
}

#endif