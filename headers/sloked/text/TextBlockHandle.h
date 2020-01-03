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

#ifndef SLOKED_TEXT_TEXTBLOCKHANDLE_H_
#define SLOKED_TEXT_TEXTBLOCKHANDLE_H_

#include "sloked/text/TextChunk.h"
#include <variant>

namespace sloked {

    class TextBlockHandle : public TextBlockImpl<TextBlockHandle> {
     public:
        TextBlockHandle(std::string_view, std::map<std::size_t, std::pair<std::size_t, std::size_t>>, const TextBlockFactory &);

        std::size_t GetLastLine() const override;
        std::size_t GetTotalLength() const override;
        std::string_view GetLine(std::size_t) const override;
        bool Empty() const override;
        void Visit(std::size_t, std::size_t, Visitor) const override;

        void SetLine(std::size_t, std::string_view) override;
        void EraseLine(std::size_t) override;
        void InsertLine(std::size_t, std::string_view) override;
        void Optimize() override;

        friend std::ostream &operator<<(std::ostream &, const TextBlockHandle &);
    
     private:
        void open_block() const;

        struct view {
            std::string_view content;
            std::map<std::size_t, std::pair<std::size_t, std::size_t>> lines;
        };

        mutable std::variant<view, std::unique_ptr<TextBlock>> content;
        const TextBlockFactory &factory;
    };
}

#endif