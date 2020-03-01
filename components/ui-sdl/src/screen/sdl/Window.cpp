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
#include "sloked/screen/sdl/Texture.h"
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

    SlokedSDLWindow::SlokedSDLWindow(SlokedSDLWindowRenderer &windowRenderer, std::unique_ptr<SlokedSDLComponent> root)
        : windowRenderer(windowRenderer), root(std::move(root)), nativeContext{nullptr}, opened{false} {}

    SlokedSDLWindow::~SlokedSDLWindow() {
        this->Close();
    }

    bool SlokedSDLWindow::IsOpen() const {
        return this->opened.load();
    }

    void SlokedSDLWindow::Open(SDL_Point dim) {
        if (!this->opened.exchange(true)) {
            this->nativeContext = std::make_unique<Context>();
            SDL_CreateWindowAndRenderer(dim.x, dim.y, 0, &this->nativeContext->window, &this->nativeContext->renderer);
            
            if (this->nativeContext->window == nullptr || this->nativeContext->renderer == nullptr) {
                this->nativeContext.reset();
                this->opened = false;
                throw SlokedError("SDLWindow: Error initializing SDL window");
            }
            this->events = SlokedSDLEventBroker::Global().Subscribe(SDL_GetWindowID(this->nativeContext->window));
            this->windowRenderer.Attach(*this);
            if (this->root) {
                this->root->SetSize(dim);
            }
        }
    }

    void SlokedSDLWindow::Close() {
        if (this->opened.exchange(false)) {
            this->windowRenderer.Detach(*this);
            this->events.reset();
            this->nativeContext.reset();
        }
    }

    SlokedSDLComponent *SlokedSDLWindow::GetRoot() const {
        std::unique_lock lock(this->mtx);
        return this->root.get();
    }

    void SlokedSDLWindow::SetRoot(std::unique_ptr<SlokedSDLComponent> root) {
        std::unique_lock lock(this->mtx);
        SDL_Point dim;
        SDL_GetWindowSize(this->nativeContext->window, &dim.x, &dim.y);
        if (this->root) {
            this->root->SetSize(dim);
        }
    }
    
    void SlokedSDLWindow::Repaint() {
        this->Render();
    }

    SDL_Point SlokedSDLWindow::Size() const {
        if (!this->opened.load()) {
            throw SlokedError("SDLWindow: Can't get size of closed window");
        }
        SDL_Point dim;
        SDL_GetWindowSize(this->nativeContext->window, &dim.x, &dim.y);
        return dim;
    }

    void SlokedSDLWindow::Resize(SDL_Point dim) {
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

    void SlokedSDLWindow::PollEvents() {
        std::unique_lock lock(this->mtx);
        if (this->root) {
            this->root->PollEvents(*this->events);
        }
    }

    void SlokedSDLWindow::Render() {
        std::unique_lock lock(this->mtx);
        SDL_RenderClear(this->nativeContext->renderer);
        if (this->root) {
            SlokedSDLSurface mainSurface(this->Size());
            this->root->Render(mainSurface);
            SlokedSDLTexture mainTexture(this->nativeContext->renderer, mainSurface);
            SDL_RenderCopy(this->nativeContext->renderer, mainTexture.GetTexture(), nullptr, nullptr);
        }
        SDL_RenderPresent(this->nativeContext->renderer);
    }

    SlokedSDLWindowRenderer::SlokedSDLWindowRenderer()
        : running{false}, framerate{std::chrono::milliseconds{50}} {}

    void SlokedSDLWindowRenderer::Start() {
        if (!this->running.exchange(true)) {
            this->renderer = std::thread([this] {
                this->Run();
            });
        }
    }

    void SlokedSDLWindowRenderer::Stop() {
        if (this->running.exchange(false)) {
            this->renderer.join();
        }
    }

    bool SlokedSDLWindowRenderer::IsRunning() const {
        return this->running.load();
    }

    void SlokedSDLWindowRenderer::Repaint() {
        this->cv.notify_all();
    }

    void SlokedSDLWindowRenderer::Attach(SlokedSDLWindow &win) {
        std::unique_lock lock(this->mtx);
        this->windows.push_back(std::ref(win));
    }

    void SlokedSDLWindowRenderer::Detach(SlokedSDLWindow &win) {
        std::unique_lock lock(this->mtx);
        this->windows.erase(std::remove_if(this->windows.begin(), this->windows.end(), [&win](auto &other) {
            return std::addressof(win) == std::addressof(other.get());
        }), this->windows.end());
    }

    void SlokedSDLWindowRenderer::Run() {
        while (this->running.load()) {
            std::unique_lock lock(this->mtx);
            for (auto win : this->windows) {
                win.get().PollEvents();
                win.get().Render();
            }
            this->cv.wait_for(lock, this->framerate);
        }
    }
}