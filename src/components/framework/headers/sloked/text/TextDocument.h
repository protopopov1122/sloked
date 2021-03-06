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

#ifndef SLOKED_TEXT_TEXTDOCUMENT_H_
#define SLOKED_TEXT_TEXTDOCUMENT_H_

#include "sloked/core/NewLine.h"
#include "sloked/text/TextBlock.h"

namespace sloked {

    class TextDocument : public TextBlockImpl<TextDocument> {
     public:
        TextDocument(const NewLine &, std::unique_ptr<TextBlock>);
        void Rebuild(const NewLine &, std::unique_ptr<TextBlock>);

        std::size_t GetLastLine() const override;
        std::size_t GetTotalLength() const override;
        std::string_view GetLine(std::size_t) const override;
        bool Empty() const override;
        void Visit(std::size_t, std::size_t, Visitor) const override;

        void SetLine(std::size_t, std::string_view) override;
        void EraseLine(std::size_t) override;
        void InsertLine(std::size_t, std::string_view) override;
        void Optimize() override;

        friend std::ostream &operator<<(std::ostream &os, const TextDocument &);

     private:
        std::reference_wrapper<const NewLine> newline;
        std::unique_ptr<TextBlock> content;
    };
}  // namespace sloked

#endif