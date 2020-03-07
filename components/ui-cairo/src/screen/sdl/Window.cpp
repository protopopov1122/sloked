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

#include "sloked/screen/sdl/Window.h"
#include "sloked/core/Error.h"

namespace sloked {

    std::atomic<uint32_t> SDLWindowCount{0};

    struct SlokedSDLWindow::Context {
        Context()
            : window{nullptr} {
            if (SDLWindowCount++ == 0) {
                if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
                    throw SlokedError("SDLWindow: Error initializing SDL");
                }
            }
        }

        ~Context() {
            SDL_DestroyWindow(this->window);
            if (--SDLWindowCount == 0) {
                SDL_Quit();
            }
        }

        SDL_Window *window;
    };

    SlokedSDLWindow::SlokedSDLWindow()
        : nativeContext{nullptr}, opened{false} {}

    SlokedSDLWindow::~SlokedSDLWindow() {
        this->Close();
    }

    bool SlokedSDLWindow::IsOpen() const {
        return this->opened.load();
    }

    void SlokedSDLWindow::Open(SlokedSDLDimensions dim, Uint32 flags) {
        if (!this->opened.exchange(true)) {
            this->nativeContext = std::make_unique<Context>();
            this->nativeContext->window = SDL_CreateWindow("",
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                dim.x,
                dim.y,
                flags);
            
            if (this->nativeContext->window == nullptr) {
                this->nativeContext.reset();
                this->opened = false;
                throw SlokedError("SDLWindow: Error initializing SDL window");
            }
            this->events = SlokedSDLEventBroker::Global().Subscribe(SDL_GetWindowID(this->nativeContext->window));
        }
    }

    void SlokedSDLWindow::Close() {
        if (this->opened.exchange(false)) {
            this->events.reset();
            this->nativeContext.reset();
        }
    }

    SlokedSDLDimensions SlokedSDLWindow::Size() const {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't get size of closed window");
        }
        SlokedSDLDimensions dim;
        SDL_GetWindowSize(this->nativeContext->window, &dim.x, &dim.y);
        return dim;
    }

    void SlokedSDLWindow::Resize(SlokedSDLDimensions dim) {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't resize closed window");
        }
        SDL_SetWindowSize(this->nativeContext->window, dim.x, dim.y);
    }

    std::string_view SlokedSDLWindow::Title() const {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't get title of closed window");
        }
        return SDL_GetWindowTitle(this->nativeContext->window);
    }

    void SlokedSDLWindow::Title(const std::string &title) {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't set title of closed window");
        }
        SDL_SetWindowTitle(this->nativeContext->window, title.c_str());
    }

    SlokedSDLEventQueue &SlokedSDLWindow::Events() const {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't get events of closed window");
        }
        return *this->events;
    }

    SDL_Window *SlokedSDLWindow::GetWindow() const {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't get window of closed window");
        }
        return this->nativeContext->window;
    }
}