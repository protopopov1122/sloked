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

#ifndef SLOKED_TEXT_FRAGMENT_TEXTFRAGMENT_H_
#define SLOKED_TEXT_FRAGMENT_TEXTFRAGMENT_H_

#include "sloked/core/Position.h"

namespace sloked {

    template <typename T>
    class TaggedTextFragment {
     public:
        TaggedTextFragment(const TextPosition &start, const TextPosition &length, T &&tag)
            : start(start), length(length), tag(std::forward<T>(tag)) {
            this->end = TextPosition{this->start.line + this->length.line, this->start.column + this->length.column};
        }

        const TextPosition &GetStart() const {
            return this->start;
        }

        const TextPosition &GetLength() const {
            return this->length;
        }

        const TextPosition &GetEnd() const {
            return this->end;
        }

        const T &GetTag() const {
            return this->tag;
        }

        bool Includes(const TextPosition &pos) const {
            return (this->start < pos || this->start == pos) &&
                pos < this->end;
        }

        bool Overlaps(const TaggedTextFragment<T> &other) const {
            const TextPosition &selfEnd = this->GetEnd();
            const TextPosition &otherEnd = other.GetEnd();
            return (this->start <= other.start && other.start < selfEnd) ||
                (other.start <= this->start && this->start < otherEnd);
        }

        bool operator<(const TaggedTextFragment<T> &other) const {
            return this->start < other.start;
        }

        bool operator==(const TaggedTextFragment<T> &other) const {
            return this->start == other.start &&
                this->length == other.length &&
                this->end == other.end &&
                this->tag == other.tag;
        }

        bool operator!=(const TaggedTextFragment<T> &other) const {
            return !this->operator==(other);
        }

     private:
        TextPosition start;
        TextPosition length;
        TextPosition end;
        T tag;
    };
}

#endif