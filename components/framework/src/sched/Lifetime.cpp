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

#include "sloked/sched/Lifetime.h"

namespace sloked {

    const std::shared_ptr<SlokedLifetime> SlokedLifetime::Global =
        std::make_shared<SlokedStandardLifetime>();

    SlokedStandardLifetime::StandardToken::~StandardToken() {
        std::unique_lock lock(this->lifetime.mtx);
        this->lifetime.tokens--;
    }

    SlokedLifetime &SlokedStandardLifetime::StandardToken::GetLifetime() const {
        return this->lifetime;
    }

    SlokedStandardLifetime::StandardToken::StandardToken(
        SlokedStandardLifetime &lifetime)
        : lifetime(lifetime) {
        this->lifetime.tokens++;
    }

    SlokedStandardLifetime::~SlokedStandardLifetime() {
        this->Close();
    }

    void SlokedStandardLifetime::Close() {
        std::unique_lock lock(this->mtx);
        if (this->active) {
            this->active = false;
            this->cv.wait(lock, [&] { return this->tokens == 0; });
        }
    }

    bool SlokedStandardLifetime::IsActive() const {
        std::unique_lock lock(this->mtx);
        return this->active;
    }

    std::unique_ptr<SlokedLifetime::Token> SlokedStandardLifetime::Acquire() {
        std::unique_lock lock(this->mtx);
        if (this->active) {
            return std::unique_ptr<StandardToken>(new StandardToken(*this));
        } else {
            return nullptr;
        }
    }
}  // namespace sloked