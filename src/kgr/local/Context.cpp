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

#include "sloked/kgr/local/Context.h"

namespace sloked {


    KgrLocalContext::KgrLocalContext(std::unique_ptr<KgrPipe> pipe)
        : pipe(std::move(pipe)) {}

    KgrLocalContext::~KgrLocalContext() {
        this->Destroy();
    }

    KgrLocalContext::State KgrLocalContext::GetState() const {
        if (this->pipe == nullptr) {
            return State::Destroyed;
        } else if (this->pipe->GetStatus() == KgrPipe::Status::Closed &&
            this->pipe->Empty()) {
            return State::Finished;
        } else if (this->pipe->Empty()) {
            return State::Idle;
        } else {
            return State::Active;
        }
    }
    
    void KgrLocalContext::Destroy() {
        this->pipe.reset();
    }

    void KgrLocalContext::SetListener(std::function<void()> callback) {
        if (this->pipe) {
            this->pipe->SetListener(std::move(callback));
        }
    }
}