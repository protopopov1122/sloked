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

#include "sloked/core/Scope.h"

namespace sloked {

    OnDestroy::OnDestroy(std::function<void()> callback)
        : callback(std::move(callback)) {}

    OnDestroy::OnDestroy(OnDestroy &&scope)
        : callback(scope.callback) {
        scope.callback = nullptr;
    }

    OnDestroy::~OnDestroy() {
        if (this->callback) {
            this->callback();
        }
    }

    OnDestroy &OnDestroy::operator=(OnDestroy &&scope) {
        if (this->callback) {
            this->callback();
        }
        this->callback = scope.callback;
        scope.callback = nullptr;
        return *this;
    }

    void OnDestroy::Detach() {
        if (this->callback) {
            this->callback();
            this->callback = nullptr;
        }
    }
}