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

#include "sloked/screen/cairo/SDL.h"
#include "sloked/screen/Keyboard.h"

namespace sloked {

    SlokedSDLCairoSurface::SlokedSDLCairoSurface(int width, int height)
        : width{width}, height{height}, sdlSurface({width, height}) {
        SDL_LockSurface(this->sdlSurface.GetSurface());
        this->cairoSurface = Cairo::ImageSurface::create(
            (unsigned char *) this->sdlSurface.GetSurface()->pixels,
            Cairo::Format::FORMAT_RGB24,
            this->sdlSurface.GetSurface()->w,
            this->sdlSurface.GetSurface()->h,
            this->sdlSurface.GetSurface()->pitch);
    }

    int SlokedSDLCairoSurface::GetWidth() const {
        return this->width;
    }

    int SlokedSDLCairoSurface::GetHeight() const {
        return this->height;
    }

    Cairo::RefPtr<Cairo::Surface> SlokedSDLCairoSurface::GetCairoSurface() const {
        return this->cairoSurface;
    }

    void SlokedSDLCairoSurface::Resize(int width, int height) {
        SlokedSDLSurface newSdlSurface({width, height});
        SDL_LockSurface(newSdlSurface.GetSurface());
        auto newSurface = Cairo::ImageSurface::create(
            (unsigned char *) newSdlSurface.GetSurface()->pixels,
            Cairo::Format::FORMAT_RGB24,
            newSdlSurface.GetSurface()->w,
            newSdlSurface.GetSurface()->h,
            newSdlSurface.GetSurface()->pitch);
        auto ctx = Cairo::Context::create(newSurface);
        ctx->set_source(this->cairoSurface, 0.0, 0.0);
        ctx->paint();
        this->cairoSurface = std::move(newSurface);
        this->sdlSurface = std::move(newSdlSurface);
        this->width = width;
        this->height = height;
    }

    std::unique_lock<std::mutex> SlokedSDLCairoSurface::Lock() {
        return std::unique_lock<std::mutex>{this->mtx};
    }

    SlokedSDLTexture SlokedSDLCairoSurface::MakeTexture(SDL_Renderer *renderer) {
        return SlokedSDLTexture(renderer, this->sdlSurface);
    }

    SlokedSDLCairoWindow::SlokedSDLCairoWindow(SlokedScreenManager &screenMgr, Dimensions dim, const std::string &title)
        : screenMgr(screenMgr), sdlWindow({dim.x, dim.y}, title), sdlRenderer(sdlWindow),
          rootSurface{dim.x, dim.y},
          root{nullptr},
          resize_request{false} {
        this->screenMgr.Attach(*this);
    }

    SlokedSDLCairoWindow::~SlokedSDLCairoWindow() {
        this->Close();
    }
    
    SlokedSDLCairoWindow::Dimensions SlokedSDLCairoWindow::GetSize() const {
        return {this->sdlWindow.Size().x, this->sdlWindow.Size().y};
    }

    void SlokedSDLCairoWindow::SetSize(Dimensions dim) {
        auto lock = this->rootSurface.Lock();
        this->sdlWindow.Resize({dim.x, dim.y});
        this->rootSurface.Resize(dim.x, dim.y);
        if (this->root) {
            this->root->SetTarget(this->rootSurface.GetCairoSurface(), dim);
        }
    }

    std::shared_ptr<SlokedCairoScreenComponent> SlokedSDLCairoWindow::GetRoot() const {
        return this->root;
    }

    void SlokedSDLCairoWindow::SetRoot(std::shared_ptr<SlokedCairoScreenComponent> root) {
        this->root = root;
        this->root->SetTarget(this->rootSurface.GetCairoSurface(), {this->rootSurface.GetWidth(), this->rootSurface.GetHeight()});
    }

    void SlokedSDLCairoWindow::Render() {
        this->PollInput();
        auto lock = this->rootSurface.Lock();
        if (this->resize_request) {
            this->resize_request = false;
            this->rootSurface.Resize(this->new_size.first, this->new_size.second);
            if (this->root) {
                this->root->SetTarget(this->rootSurface.GetCairoSurface(), {this->new_size.first, this->new_size.second});
            }
        }
        if (this->sdlWindow.IsOpen() && this->root && this->root->CheckUpdates()) {
            SDL_RenderClear(this->sdlRenderer.GetRenderer());
            if (this->root) {
                auto texture = this->rootSurface.MakeTexture(this->sdlRenderer.GetRenderer());
                SDL_RenderCopy(this->sdlRenderer.GetRenderer(), texture.GetTexture(), nullptr, nullptr); 
            }
            SDL_RenderPresent(this->sdlRenderer.GetRenderer());
        }
    }

    void SlokedSDLCairoWindow::Close() {
        if (this->sdlWindow.IsOpen()) {
            this->screenMgr.Detach(*this);
            this->root = nullptr;
            this->sdlRenderer = {nullptr};
            this->sdlWindow.Close();
        }
    }

    void SlokedSDLCairoWindow::PollInput() {
        static std::map<SDL_Keycode, SlokedControlKey> KeyMappings = {
            { SDL_SCANCODE_UP, SlokedControlKey::ArrowUp },
            { SDL_SCANCODE_DOWN, SlokedControlKey::ArrowDown },
            { SDL_SCANCODE_LEFT, SlokedControlKey::ArrowLeft },
            { SDL_SCANCODE_RIGHT, SlokedControlKey::ArrowRight },
            { SDL_SCANCODE_BACKSPACE, SlokedControlKey::Backspace },
            { SDL_SCANCODE_DELETE, SlokedControlKey::Delete },
            { SDL_SCANCODE_INSERT, SlokedControlKey::Insert },
            { SDL_SCANCODE_ESCAPE, SlokedControlKey::Escape },
            { SDL_SCANCODE_PAGEUP, SlokedControlKey::PageUp },
            { SDL_SCANCODE_PAGEDOWN, SlokedControlKey::PageDown },
            { SDL_SCANCODE_HOME, SlokedControlKey::Home },
            { SDL_SCANCODE_END, SlokedControlKey::End },
            { SDL_SCANCODE_RETURN, SlokedControlKey::Enter },
            { SDL_SCANCODE_TAB, SlokedControlKey::Tab },
            { SDL_SCANCODE_F1, SlokedControlKey::F1 },
            { SDL_SCANCODE_F2, SlokedControlKey::F2 },
            { SDL_SCANCODE_F3, SlokedControlKey::F3 },
            { SDL_SCANCODE_F4, SlokedControlKey::F4 },
            { SDL_SCANCODE_F5, SlokedControlKey::F5 },
            { SDL_SCANCODE_F6, SlokedControlKey::F6 },
            { SDL_SCANCODE_F7, SlokedControlKey::F7 },
            { SDL_SCANCODE_F8, SlokedControlKey::F8 },
            { SDL_SCANCODE_F9, SlokedControlKey::F9 },
            { SDL_SCANCODE_F10, SlokedControlKey::F10 },
            { SDL_SCANCODE_F11, SlokedControlKey::F11 },
            { SDL_SCANCODE_F12, SlokedControlKey::F12 }
        };

        static std::map<SDL_Keycode, SlokedControlKey> ControlKeyMappings = {
            { SDL_SCANCODE_SPACE, SlokedControlKey::CtrlSpace },
            { SDL_SCANCODE_SPACE, SlokedControlKey::CtrlA },
            { SDL_SCANCODE_SPACE, SlokedControlKey::CtrlB },
            { SDL_SCANCODE_D, SlokedControlKey::CtrlD },
            { SDL_SCANCODE_E, SlokedControlKey::CtrlE },
            { SDL_SCANCODE_F, SlokedControlKey::CtrlF },
            { SDL_SCANCODE_G, SlokedControlKey::CtrlG },
            { SDL_SCANCODE_H, SlokedControlKey::CtrlH },
            { SDL_SCANCODE_K, SlokedControlKey::CtrlK },
            { SDL_SCANCODE_L, SlokedControlKey::CtrlL },
            { SDL_SCANCODE_N, SlokedControlKey::CtrlN },
            { SDL_SCANCODE_O, SlokedControlKey::CtrlO },
            { SDL_SCANCODE_P, SlokedControlKey::CtrlP },
            { SDL_SCANCODE_R, SlokedControlKey::CtrlR },
            { SDL_SCANCODE_T, SlokedControlKey::CtrlT },
            { SDL_SCANCODE_U, SlokedControlKey::CtrlU },
            { SDL_SCANCODE_V, SlokedControlKey::CtrlV },
            { SDL_SCANCODE_W, SlokedControlKey::CtrlW },
            { SDL_SCANCODE_X, SlokedControlKey::CtrlX },
            { SDL_SCANCODE_Y, SlokedControlKey::CtrlY }
        };

        if (this->sdlWindow.IsOpen()) {
            std::vector<SlokedKeyboardInput> events;
            auto &eventSource = this->sdlWindow.Events();
            while (eventSource.HasEvents()) {
                auto evt = eventSource.NextEvent();
                switch (evt.type) {
                    case SDL_KEYDOWN:
                        if ((evt.key.keysym.mod & KMOD_CTRL) == 0 && KeyMappings.count(evt.key.keysym.scancode) != 0) {
                            events.emplace_back(SlokedKeyboardInput {
                                KeyMappings.at(evt.key.keysym.scancode),
                                (evt.key.keysym.mod & KMOD_ALT) != 0
                            });
                        } else if ((evt.key.keysym.mod & KMOD_CTRL) != 0 && ControlKeyMappings.count(evt.key.keysym.scancode) != 0) {
                            events.emplace_back(SlokedKeyboardInput {
                                ControlKeyMappings.at(evt.key.keysym.scancode),
                                (evt.key.keysym.mod & KMOD_ALT) != 0
                            });
                        }
                        break;

                    case SDL_TEXTEDITING:
                        events.emplace_back(SlokedKeyboardInput {
                            std::string{evt.edit.text},
                            false
                        });
                        break;

                    case SDL_TEXTINPUT:
                        events.emplace_back(SlokedKeyboardInput {
                            std::string{evt.text.text},
                            false
                        });
                        break;

                    case SDL_WINDOWEVENT:
                        switch (evt.window.event) {
                            case SDL_WINDOWEVENT_RESIZED: {
                                this->new_size = std::make_pair(evt.window.data1, evt.window.data2);
                                this->resize_request = true;
                            } break;
                        }
                        break;

                    default:
                        break;
                }
            }
            if (!events.empty() && this->root) {
                this->root->ProcessInput(std::move(events));
            }
        }
    }
}