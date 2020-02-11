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

#ifndef SLOKED_KGR_SERIALIZE_H_
#define SLOKED_KGR_SERIALIZE_H_

#include "sloked/core/Encoding.h"
#include "sloked/kgr/Value.h"
#include "sloked/json/AST.h"
#include <vector>

namespace sloked {

    class KgrSerializer {
     public:
        using Blob = std::vector<char>;

        virtual ~KgrSerializer() = default;
        virtual Blob Serialize(const KgrValue &) const = 0;
        virtual KgrValue Deserialize(const Blob &) const = 0;
        virtual KgrValue Deserialize(std::istream &) const = 0;
    };

    class KgrJsonSerializer : public KgrSerializer {
     public:
        KgrJsonSerializer(const Encoding & = Encoding::Utf8);
        Blob Serialize(const KgrValue &) const override;
        KgrValue Deserialize(const Blob &) const override;
        KgrValue Deserialize(std::istream &) const override;

     private:
        std::unique_ptr<JsonASTNode> SerializeValue(const KgrValue &) const;
        KgrValue DeserializeValue(const JsonASTNode &) const;

        const Encoding &encoding;
    };
}

#endif