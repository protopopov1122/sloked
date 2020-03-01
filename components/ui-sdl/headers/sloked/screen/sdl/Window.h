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
#include "sloked/screen/sdl/Component.h"
#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace sloked {

    class SlokedSDLWindowRenderer;

    class SlokedSDLWindow {
     public:
        SlokedSDLWindow(SlokedSDLWindowRenderer &, std::unique_ptr<SlokedSDLComponent> = nullptr);
        ~SlokedSDLWindow();

        bool IsOpen() const;
        void Open(SDL_Point);
        void Close();
        SlokedSDLComponent *GetRoot() const;
        void SetRoot(std::unique_ptr<SlokedSDLComponent>);
        void Repaint();
        SDL_Point Size() const;
        void Resize(SDL_Point);
        std::string_view Title() const;
        void Title(const std::string &);
        SlokedSDLEventQueue &Events() const;

        friend class SlokedSDLWindowRenderer;

     private:
        struct Context;
        void Init(SDL_Point);
        void PollEvents();
        void Render();

        SlokedSDLWindowRenderer &windowRenderer;
        std::unique_ptr<SlokedSDLComponent> root;
        std::unique_ptr<Context> nativeContext;
        mutable std::mutex mtx;
        std::atomic_bool opened;
        std::unique_ptr<SlokedSDLEventQueue> events;
    };

    class SlokedSDLWindowRenderer {
     public:
        SlokedSDLWindowRenderer();
        void Start();
        void Stop();
        bool IsRunning() const;
        void Repaint();
        void Attach(SlokedSDLWindow &);
        void Detach(SlokedSDLWindow &);

     private:
        void Run();
        
        std::atomic_bool running;
        std::vector<std::reference_wrapper<SlokedSDLWindow>> windows;
        std::mutex mtx;
        std::condition_variable cv;
        std::thread renderer;
        std::chrono::system_clock::duration framerate;
    };
}

#endif