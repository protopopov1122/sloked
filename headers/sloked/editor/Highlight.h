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

#ifndef SLOKED_EDITOR_HIGHLIGHT_H_
#define SLOKED_EDITOR_HIGHLIGHT_H_

#include "sloked/text/TextFrame.h"
#include "sloked/text/fragment/TaggedText.h"

namespace sloked {

    template <typename T>
    class SlokedTaggedFrame {
     public:
        struct TaggedFragmentContent {
            std::string_view content;
            const TaggedTextFragment<T> *tag;
        };
        using TaggedLine = std::vector<TaggedFragmentContent>;

        SlokedTaggedFrame(const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth, SlokedTaggedText<T> &tags)
            : text(text, encoding, charWidth), tags(tags) {}

        void Update(const TextPosition &offset, const TextPosition &dim) {
            this->content.clear();
            this->text.Update(offset, dim);
            for (TextPosition::Line line = 0; !text.Empty() && line <= text.GetLastLine(); line++) {
                content.push_back(TaggedLine{});
                TaggedLine &currentLine = content.back();
                std::string_view currentContent = text.GetLine(line);
                // for (TextPosition::Column col = 0; c9o)w
            }
        }

     private:
        TextFrameView text;
        SlokedTaggedText<T> &tags;
        std::vector<TaggedLine> content;
    };
}

#endif