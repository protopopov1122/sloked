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

#include "sloked/text/search/Replace.h"
#include <iostream>

namespace sloked {

    SlokedTextReplacer::SlokedTextReplacer(TextBlock &text, const Encoding &encoding)
        : text(text), encoding(encoding) {}

    void SlokedTextReplacer::Replace(const SlokedSearchEntry &entry, std::string_view value) {
        std::string currentLine{this->text.GetLine(entry.start.line)};
        auto start = this->encoding.GetCodepoint(currentLine, entry.start.column).first;
        auto end = this->encoding.GetCodepoint(currentLine, entry.start.column + entry.length).first;
        currentLine.replace(start, end - start, value);
        this->text.SetLine(entry.start.line, currentLine);
    }
}