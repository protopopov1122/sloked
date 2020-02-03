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

#ifndef SLOKED_SCREEN_UI_SDL_H_
#define SLOKED_SCREEN_UI_SDL_H_

#include "sloked/Base.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <SDL2/SDL.h>

namespace sloked {

    class SlokedSDLWindow {
     public:
        using Dimension = int;
        using Renderer = std::function<void(SDL_Window *, SDL_Renderer *)>;

        SlokedSDLWindow(Renderer = nullptr);
        ~SlokedSDLWindow();

        bool IsOpen() const;
        void Open(const std::string &, Dimension, Dimension);
        void Close();
        void SetRenderer(Renderer);
        void Repaint();
        std::pair<Dimension, Dimension> Size() const;
        void Resize(Dimension, Dimension);

     private:
        struct Context;
        void Init(const std::string &, Dimension, Dimension);
        void Run();
        
        std::atomic<bool> running;
        std::thread worker;
        std::unique_ptr<Context> nativeContext;
        std::mutex mtx;
        std::condition_variable cond;
        Renderer renderer;
    };
}

#endif