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

#include "sloked/screen/sdl/Event.h"
#include "sloked/core/Error.h"

namespace sloked {

    bool SlokedSDLGlobalEventQueue::HasEvents() const {
        return SDL_PollEvent(nullptr);
    }

    SDL_Event SlokedSDLGlobalEventQueue::NextEvent() {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            return event;
        } else {
            throw SlokedError("SDLEventQueue: No pending events");
        }
    }

    void SlokedSDLGlobalEventQueue::EnableText(bool enable) const {
        if (enable) {
            SDL_StartTextInput();
        } else {
            SDL_StopTextInput();
        }
    }

    SlokedSDLGlobalEventQueue &SlokedSDLGlobalEventQueue::Get() {
        static SlokedSDLGlobalEventQueue globalQueue;
        return globalQueue;
    }

    SlokedSDLGlobalEventQueue::SlokedSDLGlobalEventQueue() {}

    class SlokedSDLEventBroker::GlobalQueue : public SlokedSDLEventQueue {
     public:
        GlobalQueue(SlokedSDLEventBroker &broker)
            : broker(broker) {}
        
        bool HasEvents() const final {
            std::unique_lock lock(this->broker.mtx);
            this->broker.PollEvents();
            return !this->broker.globalQueue.empty();
        }
        
        SDL_Event NextEvent() final {
            std::unique_lock lock(this->broker.mtx);
            this->broker.PollEvents();
            if (!this->broker.globalQueue.empty()) {
                auto event = std::move(this->broker.globalQueue.front());
                this->broker.globalQueue.pop();
                return event;
            } else {
                throw SlokedError("SDLEventQueue: No pending events");
            }
        }

     private:
        SlokedSDLEventBroker &broker;
    };

    class SlokedSDLEventBroker::WindowQueue : public SlokedSDLEventQueue {
     public:
        WindowQueue(SlokedSDLEventBroker &broker, WindowID id)
            : broker(broker), winId(id) {
            std::unique_lock lock(this->broker.mtx);
            if (this->broker.windows.count(id) == 0) {
                this->broker.windows.emplace(id, std::queue<SDL_Event>{});
            } else {
                throw SlokedError("SDLEventBroker: Window queue already registered");
            }
        }

        ~WindowQueue() {
            std::unique_lock lock(this->broker.mtx);
            this->broker.windows.erase(this->winId);
        }

        bool HasEvents() const final {
            std::unique_lock lock(this->broker.mtx);
            this->broker.PollEvents();
            return !this->broker.windows.at(this->winId).empty();
        }

        SDL_Event NextEvent() final {
            std::unique_lock lock(this->broker.mtx);
            this->broker.PollEvents();
            if (!this->broker.windows.at(this->winId).empty()) {
                auto event = std::move(this->broker.windows.at(this->winId).front());
                this->broker.windows.at(this->winId).pop();
                return event;
            } else {
                throw SlokedError("SDLEventQueue: No pending events");       
            }
        }

     private:
        SlokedSDLEventBroker &broker;
        WindowID winId;
    };

    SlokedSDLEventBroker::SlokedSDLEventBroker(SlokedSDLEventQueue &source)
        : source(source), global(std::make_unique<GlobalQueue>(*this)) {}

    SlokedSDLEventBroker::~SlokedSDLEventBroker() = default;

    std::unique_ptr<SlokedSDLEventQueue> SlokedSDLEventBroker::Subscribe(WindowID id) {
        return std::make_unique<WindowQueue>(*this, id);
    }
    
    SlokedSDLEventQueue &SlokedSDLEventBroker::Common() {
        return *this->global;
    }

    SlokedSDLEventBroker &SlokedSDLEventBroker::Global() {
        static SlokedSDLEventBroker broker(SlokedSDLGlobalEventQueue::Get());
        return broker;
    }

    void SlokedSDLEventBroker::PollEvents() {
        while (this->source.HasEvents()) {
            auto event = this->source.NextEvent();
            switch (event.type) {
                case SDL_DROPFILE:
                case SDL_DROPTEXT:
                case SDL_DROPBEGIN:
                case SDL_DROPCOMPLETE:
                    this->AttachEvent(event.drop.windowID, std::move(event));
                    break;

                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    this->AttachEvent(event.key.windowID, std::move(event));
                    break;

                case SDL_MOUSEMOTION:
                    this->AttachEvent(event.motion.windowID, std::move(event));
                    break;

                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    this->AttachEvent(event.button.windowID, std::move(event));
                    break;

                case SDL_MOUSEWHEEL:
                    this->AttachEvent(event.wheel.windowID, std::move(event));
                    break;
                
                case SDL_TEXTEDITING:
                    this->AttachEvent(event.edit.windowID, std::move(event));
                    break;
                
                case SDL_TEXTINPUT:
                    this->AttachEvent(event.text.windowID, std::move(event));
                    break;

                case SDL_USEREVENT:
                    this->AttachEvent(event.user.windowID, std::move(event));
                    break;

                case SDL_WINDOWEVENT:
                    this->AttachEvent(event.window.windowID, std::move(event));
                    break;

                default:
                    this->globalQueue.push(std::move(event));
                    break;
            }
        }
    }

    void SlokedSDLEventBroker::AttachEvent(WindowID winId, SDL_Event evt) {
        if (this->windows.count(winId) != 0) {
            this->windows.at(winId).push(std::move(evt));
        } else {
            this->globalQueue.push(std::move(evt));
        }
    }
}