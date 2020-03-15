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

#ifndef SLOKED_SCREEN_SDL_WINDOW_H_
#define SLOKED_SCREEN_SDL_WINDOW_H_

#include "sloked/screen/sdl/Event.h"
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace sloked {

    class SlokedSDLWindow {
     public:
        SlokedSDLWindow();
        SlokedSDLWindow(SlokedSDLDimensions, const std::string & = "", Uint32 = 0);
        ~SlokedSDLWindow();

        bool IsOpen() const;
        void Open(SlokedSDLDimensions, const std::string & = "", Uint32 = 0);
        void Close();
        SlokedSDLDimensions Size() const;
        void Resize(SlokedSDLDimensions);
        std::string_view Title() const;
        void Title(const std::string &);
        SlokedSDLEventQueue &Events() const;
        SDL_Window *GetWindow() const;

     private:
        SDL_Window *window;
        std::atomic_bool opened;
        std::unique_ptr<SlokedSDLEventQueue> events;
    };
}

#endif