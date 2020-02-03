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

#include "sloked/screen/ui/SDL.h"
#include "sloked/core/Error.h"
#include <chrono>
#include <SDL2/SDL.h>

namespace sloked {

    std::atomic<uint32_t> SDLWindowCount{0};

    struct SlokedSDLWindow::Context {
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
        : running{false}, nativeContext{nullptr}, renderer{std::move(rend)} {}

    SlokedSDLWindow::~SlokedSDLWindow() {
        this->Close();
    }

    bool SlokedSDLWindow::IsOpen() const {
        return this->running.load();
    }

    void SlokedSDLWindow::Open(const std::string &title, Dimension width, Dimension height) {
        if (!this->running.exchange(true)) {
            try {
                this->Init(title, width, height);
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

    std::pair<SlokedSDLWindow::Dimension, SlokedSDLWindow::Dimension> SlokedSDLWindow::Size() const {
        if (!this->running.load()) {
            throw SlokedError("SDLWindow: Can't get size of closed window");
        }
        Dimension width, height;
        SDL_GetWindowSize(this->nativeContext->window, &width, &height);
        return std::make_pair(width, height);
    }

    void SlokedSDLWindow::Resize(Dimension width, Dimension height) {
        if (!this->running.load()) {
            throw SlokedError("SDLWindow: Can't resize closed window");
        }
        SDL_SetWindowSize(this->nativeContext->window, width, height);
    }

    void SlokedSDLWindow::Init(const std::string &title, Dimension width, Dimension height) {
        this->nativeContext = std::make_unique<Context>();
        if (SDLWindowCount++ == 0 && SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            throw SlokedError("SDLWindow: Error initializing SDL");
        }
        
        SDL_Window* window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);
        if (window == nullptr) {
            this->nativeContext.reset();
            throw SlokedError("SDLWindow: Error initializing SDL window");
        }
        this->nativeContext->window = std::move(window);
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (renderer == nullptr) {
            this->nativeContext.reset();
            throw SlokedError("SDLWindow: Error initializing SDL renderer");
        }
        this->nativeContext->renderer = std::move(renderer);
    }

    void SlokedSDLWindow::Run() {
        constexpr auto MaxDelay = std::chrono::milliseconds(100);
        while (this->running.load()) {
            std::unique_lock lock(this->mtx);
            SDL_RenderClear(this->nativeContext->renderer);
            if (this->renderer) {
                this->renderer(this->nativeContext->window, this->nativeContext->renderer);
            }
            SDL_RenderPresent(this->nativeContext->renderer);
            this->cond.wait_for(lock, MaxDelay);
        }
    }
}