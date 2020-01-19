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

#ifndef SLOKED_EDITOR_CONFIGURATION_H_
#define SLOKED_EDITOR_CONFIGURATION_H_

#include "sloked/kgr/Value.h"

namespace sloked {

    class SlokedConfigurationLoader {
     public:
        virtual ~SlokedConfigurationLoader() = default;
        virtual KgrValue Load() const = 0;
        static KgrValue LoadFile(const std::string &);
    };

    class SlokedXdgConfigurationLoader : public SlokedConfigurationLoader {
     public:
        SlokedXdgConfigurationLoader(const std::string &);
        KgrValue Load() const final;

     private:
        std::string name;
    };

    class SlokedConfiguration {
     public:
        SlokedConfiguration(std::initializer_list<KgrValue>);
        KgrValue Find(const std::string &) const;
        bool Has(const std::string &) const;

     private:
        std::vector<KgrValue> layers;
    };
}

#endif