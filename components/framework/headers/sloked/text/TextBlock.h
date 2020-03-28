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

#ifndef SLOKED_TEXT_TEXTBLOCK_H_
#define SLOKED_TEXT_TEXTBLOCK_H_

#include <cinttypes>
#include <functional>
#include <iosfwd>
#include <memory>
#include <string>

#include "sloked/Base.h"

namespace sloked {

    class TextBlockView {
     public:
        using Visitor = std::function<void(std::string_view)>;

        virtual ~TextBlockView() = default;

        virtual std::size_t GetLastLine() const = 0;
        virtual std::size_t GetTotalLength() const = 0;
        virtual std::string_view GetLine(std::size_t) const = 0;
        virtual bool Empty() const = 0;
        virtual void Visit(std::size_t, std::size_t, Visitor) const;

        friend std::ostream &operator<<(std::ostream &os,
                                        const TextBlockView &block) {
            return block.dump(os);
        }

     protected:
        virtual std::ostream &dump(std::ostream &) const = 0;
    };

    class TextBlock : public TextBlockView {
     public:
        virtual void SetLine(std::size_t, std::string_view) = 0;
        virtual void EraseLine(std::size_t) = 0;
        virtual void InsertLine(std::size_t, std::string_view) = 0;
        virtual void Optimize();
    };

    template <typename T>
    class TextBlockImpl : public TextBlock {
     protected:
        std::ostream &dump(std::ostream &os) const override {
            return os << *static_cast<const T *>(this);
        }
    };

    class TextBlockFactory {
     public:
        virtual ~TextBlockFactory() = default;
        virtual std::unique_ptr<TextBlock> make(std::string_view) const = 0;
    };
}  // namespace sloked

#endif