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
            : window{nullptr}, renderer{nullptr} {
            if (SDLWindowCount++ == 0 && SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
                throw SlokedError("SDLWindow: Error initializing SDL");
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

    SlokedSDLWindow::SlokedSDLWindow(Renderer rend)
        : running{false}, nativeContext{nullptr}, repaint_delay{std::chrono::milliseconds(100)}, renderer{std::move(rend)} {}

    SlokedSDLWindow::~SlokedSDLWindow() {
        this->Close();
    }

    bool SlokedSDLWindow::IsOpen() const {
        return this->running.load();
    }

    void SlokedSDLWindow::Open(SDL_Point dim) {
        if (!this->running.exchange(true)) {
            try {
                this->Init(std::move(dim));
                this->worker = std::thread([this] {
                    this->Run();
                });
            } catch (...) {
                this->running = false;
                throw;
            }
        }
    }

    void SlokedSDLWindow::Close() {
        if (this->running.exchange(false)) {
            this->worker.join();
            this->nativeContext.reset();
        }
    }

    void SlokedSDLWindow::SetRenderer(Renderer rend) {
        std::unique_lock lock(this->mtx);
        this->renderer = std::move(rend);
        this->cond.notify_all();
    }
    
    void SlokedSDLWindow::Repaint() {
        this->cond.notify_all();
    }

    SDL_Point SlokedSDLWindow::Size() const {
        if (!this->running.load()) {
            throw SlokedError("SDLWindow: Can't get size of closed window");
        }
        SDL_Point dim;
        SDL_GetWindowSize(this->nativeContext->window, &dim.x, &dim.y);
        return dim;
    }

    void SlokedSDLWindow::Resize(SDL_Point dim) {
        if (!this->running.load()) {
            throw SlokedError("SDLWindow: Can't resize closed window");
        }
        SDL_SetWindowSize(this->nativeContext->window, dim.x, dim.y);
    }

    std::string_view SlokedSDLWindow::Title() const {
        if (!this->running.load()) {
            throw SlokedError("SDLWindow: Can't get title of closed window");
        }
        return SDL_GetWindowTitle(this->nativeContext->window);
    }

    void SlokedSDLWindow::Title(const std::string &title) {
        if (!this->running.load()) {
            throw SlokedError("SDLWindow: Can't set title of closed window");
        }
        SDL_SetWindowTitle(this->nativeContext->window, title.c_str());
    }

    void SlokedSDLWindow::Init(SDL_Point dim) {
        this->nativeContext = std::make_unique<Context>();
        SDL_CreateWindowAndRenderer(dim.x, dim.y, 0, &this->nativeContext->window, &this->nativeContext->renderer);
        
        if (this->nativeContext->window == nullptr || this->nativeContext->renderer == nullptr) {
            this->nativeContext.reset();
            throw SlokedError("SDLWindow: Error initializing SDL window");
        }
    }

    void SlokedSDLWindow::Run() {
        while (this->running.load()) {
            std::unique_lock lock(this->mtx);
            SDL_RenderClear(this->nativeContext->renderer);
            if (this->renderer) {
                this->renderer(this->nativeContext->window, this->nativeContext->renderer);
            }
            SDL_RenderPresent(this->nativeContext->renderer);
            this->cond.wait_for(lock, this->repaint_delay);
        }
    }
}