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

#ifndef SLOKED_SCREEN_TERMINAL_TERMINALSIZE_H_
#define SLOKED_SCREEN_TERMINAL_TERMINALSIZE_H_

#include "sloked/screen/Size.h"
#include "sloked/screen/terminal/Terminal.h"
#include <mutex>
#include <map>

namespace sloked {

    class SlokedTerminalSize : public SlokedScreenSize {
     public:
        SlokedTerminalSize(SlokedTerminal &);
        ~SlokedTerminalSize();
        TextPosition GetSize() const final;
        std::function<void()> Listen(Listener) final;

     private:
        void Trigger();
        
        SlokedTerminal &terminal;
        std::mutex mtx;
        int64_t nextId;
        std::map<int64_t, Listener> listeners;
        std::function<void()> unsubscribe;
    };
}

#endif