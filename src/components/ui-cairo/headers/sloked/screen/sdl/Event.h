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

#ifndef SLOKED_SCREEN_SDL_EVENT_H_
#define SLOKED_SCREEN_SDL_EVENT_H_

#include <map>
#include <memory>
#include <mutex>
#include <queue>

#include "sloked/screen/sdl/Base.h"

namespace sloked {

    class SlokedSDLEventQueue {
     public:
        virtual ~SlokedSDLEventQueue() = default;
        virtual bool HasEvents() const = 0;
        virtual SDL_Event NextEvent() = 0;
    };

    class SlokedSDLGlobalEventQueue : public SlokedSDLEventQueue {
     public:
        bool HasEvents() const final;
        SDL_Event NextEvent() final;
        void EnableText(bool = true) const;

        static SlokedSDLGlobalEventQueue &Get();

     private:
        SlokedSDLGlobalEventQueue();
    };

    class SlokedSDLEventBroker {
     public:
        using WindowID = Uint32;
        SlokedSDLEventBroker(SlokedSDLEventQueue &);
        ~SlokedSDLEventBroker();
        std::unique_ptr<SlokedSDLEventQueue> Subscribe(WindowID);
        SlokedSDLEventQueue &Common();

        static SlokedSDLEventBroker &Global();

     private:
        class GlobalQueue;
        class WindowQueue;
        friend class GlobalQueue;
        friend class WindowQueue;

        void PollEvents();
        void AttachEvent(WindowID, SDL_Event);

        SlokedSDLEventQueue &source;
        std::unique_ptr<GlobalQueue> global;
        std::mutex mtx;
        std::queue<SDL_Event> globalQueue;
        std::map<WindowID, std::queue<SDL_Event>> windows;
    };
}  // namespace sloked

#endif