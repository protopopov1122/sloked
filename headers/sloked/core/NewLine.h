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

#ifndef SLOKED_CORE_NEWLINE_H_
#define SLOKED_CORE_NEWLINE_H_

#include "sloked/Base.h"
#include "sloked/core/Encoding.h"
#include <string>
#include <functional>
#include <memory>
#include <iosfwd>

namespace sloked {

    class NewLine {
     public:
        using Iterator = std::function<void(std::size_t, std::size_t)>;

        NewLine(const std::string &);
        NewLine(const NewLine &) = delete;
        NewLine(NewLine &&) = delete;
        virtual ~NewLine() = default;

        NewLine &operator=(const NewLine &) = delete;
        NewLine &operator=(NewLine &&) = delete;

        virtual void Iterate(std::string_view, Iterator) const = 0;
        virtual std::size_t Count(std::string_view) const = 0;

        const std::size_t Width;
        static std::unique_ptr<NewLine> LF(const Encoding &);
        static std::unique_ptr<NewLine> CRLF(const Encoding &);
        static std::unique_ptr<NewLine> Create(const std::string &, const Encoding &);
    
     private:
        friend std::ostream &operator<<(std::ostream &, const NewLine &);
        std::string symbol;
    };
}

#endif