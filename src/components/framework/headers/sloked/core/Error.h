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

#ifndef SLOKED_CORE_ERROR_H_
#define SLOKED_CORE_ERROR_H_

#include <exception>
#include <string>

#include "sloked/Base.h"

namespace sloked {

    class SlokedError : public std::exception {
     public:
        SlokedError(std::string_view);
        const char *what() const noexcept override;

     private:
        std::string message;
    };

    class SlokedServiceError : public SlokedError {
     public:
        using SlokedError::SlokedError;
    };
}  // namespace sloked

#endif