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
#include <chrono>
#include <SDL2/SDL.h>

namespace sloked {

    using SlokedSDLDimension = int;

    class SlokedSDLWindow {
     public:
        using Dimension = SlokedSDLDimension;
        using Renderer = std::function<void(SDL_Window *, SDL_Renderer *)>;

        SlokedSDLWindow(Renderer = nullptr);
        ~SlokedSDLWindow();

        bool IsOpen() const;
        void Open(Dimension, Dimension);
        void Close();
        void SetRenderer(Renderer);
        void Repaint();
        SDL_Point Size() const;
        void Resize(SDL_Point);
        std::string_view Title() const;
        void Title(const std::string &);

     private:
        struct Context;
        void Init(Dimension, Dimension);
        void Run();
        
        std::atomic<bool> running;
        std::thread worker;
        std::unique_ptr<Context> nativeContext;
        std::mutex mtx;
        std::condition_variable cond;
        std::chrono::system_clock::duration repaint_delay;
        Renderer renderer;
    };

    struct SlokedSDLColor {
        using Value = Uint32;
        using Component = Uint8;

        Component red;
        Component green;
        Component blue;
        Component alpha{0};
    };

    class SlokedSDLSurface {
     public:
        SlokedSDLSurface(SDL_Surface * = nullptr);
        SlokedSDLSurface(SDL_Point);
        SlokedSDLSurface(const SlokedSDLSurface &) = delete;
        SlokedSDLSurface(SlokedSDLSurface &&);
        ~SlokedSDLSurface();

        SlokedSDLSurface &operator=(const SlokedSDLSurface &) = delete;
        SlokedSDLSurface &operator=(SlokedSDLSurface &&);

        SDL_Surface *GetSurface() const;
        SDL_Point Size() const;
        SlokedSDLColor::Value MapColor(SlokedSDLColor) const;
        SlokedSDLColor MapColor(SlokedSDLColor::Value) const;
        void Fill(SDL_Rect, SlokedSDLColor) const;

     private:
        SDL_Surface *surface;
    };

    class SlokedSDLTexture {
     public:
        SlokedSDLTexture(SDL_Texture *);
        SlokedSDLTexture(SDL_Renderer *, const SlokedSDLSurface &);
        SlokedSDLTexture(const SlokedSDLTexture &) = delete;
        SlokedSDLTexture(SlokedSDLTexture &&);
        ~SlokedSDLTexture();

        SlokedSDLTexture &operator=(const SlokedSDLTexture &) = delete;
        SlokedSDLTexture &operator=(SlokedSDLTexture &&);

        SDL_Texture *GetTexture() const;

     private:
        SDL_Texture *texture;
    };
}

#endif