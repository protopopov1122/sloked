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

#include "sloked/screen/cairo/sdl/Window.h"
#include "sloked/core/Error.h"

namespace sloked {

    std::atomic<uint32_t> SDLWindowCount{0};

    struct SlokedCairoSDLWindow::Context {
        Context()
            : window{nullptr} {
            if (SDLWindowCount++ == 0) {
                if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
                    throw SlokedError("SDLWindow: Error initializing SDL");
                }
            }
        }

        ~Context() {
            SDL_DestroyRenderer(this->renderer);
            SDL_DestroyWindow(this->window);
            if (--SDLWindowCount == 0) {
                SDL_Quit();
            }
        }

        SDL_Window *window;
        SDL_Renderer *renderer;
    };

    SlokedCairoSDLWindow::SlokedCairoSDLWindow()
        : nativeContext{nullptr}, opened{false} {}

    SlokedCairoSDLWindow::~SlokedCairoSDLWindow() {
        this->Close();
    }

    bool SlokedCairoSDLWindow::IsOpen() const {
        return this->opened.load();
    }

    void SlokedCairoSDLWindow::Open(SDL_Point dim) {
        if (!this->opened.exchange(true)) {
            this->nativeContext = std::make_unique<Context>();
            SDL_CreateWindowAndRenderer(dim.x, dim.y, 0, &this->nativeContext->window, &this->nativeContext->renderer);
            
            if (this->nativeContext->window == nullptr || this->nativeContext->renderer == nullptr) {
                this->nativeContext.reset();
                this->opened = false;
                throw SlokedError("SDLWindow: Error initializing SDL window");
            }
            this->events = SlokedCairoSDLEventBroker::Global().Subscribe(SDL_GetWindowID(this->nativeContext->window));
        }
    }

    void SlokedCairoSDLWindow::Close() {
        if (this->opened.exchange(false)) {
            this->events.reset();
            this->nativeContext.reset();
        }
    }

    SDL_Point SlokedCairoSDLWindow::Size() const {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't get size of closed window");
        }
        SDL_Point dim;
        SDL_GetWindowSize(this->nativeContext->window, &dim.x, &dim.y);
        return dim;
    }

    void SlokedCairoSDLWindow::Resize(SDL_Point dim) {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't resize closed window");
        }
        SDL_SetWindowSize(this->nativeContext->window, dim.x, dim.y);
    }

    std::string_view SlokedCairoSDLWindow::Title() const {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't get title of closed window");
        }
        return SDL_GetWindowTitle(this->nativeContext->window);
    }

    void SlokedCairoSDLWindow::Title(const std::string &title) {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't set title of closed window");
        }
        SDL_SetWindowTitle(this->nativeContext->window, title.c_str());
    }

    SlokedCairoSDLEventQueue &SlokedCairoSDLWindow::Events() const {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't get events of closed window");
        }
        return *this->events;
    }

    SDL_Window *SlokedCairoSDLWindow::GetWindow() const {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't get window of closed window");
        }
        return this->nativeContext->window;
    }

    SDL_Renderer *SlokedCairoSDLWindow::GetRenderer() const {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't get renderer of closed window");
        }
        return this->nativeContext->renderer;
    }
}