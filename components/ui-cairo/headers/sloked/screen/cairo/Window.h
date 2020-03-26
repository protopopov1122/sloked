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

#ifndef SLOKED_SCREEN_CAIRO_WINDOW_H_
#define SLOKED_SCREEN_CAIRO_WINDOW_H_

#include "sloked/screen/cairo/Component.h"
#include "sloked/screen/graphics/Window.h"
#include "sloked/screen/Manager.h"
#include <memory>

namespace sloked {

    class SlokedAbstractCairoWindow : public SlokedScreenManager::Renderable {
     public:
        using Dimensions = SlokedGraphicsDimensions;

        virtual ~SlokedAbstractCairoWindow() = default;
        virtual Dimensions GetSize() const = 0;
        virtual void SetSize(Dimensions) = 0;
        virtual std::shared_ptr<SlokedCairoScreenComponent> GetRoot() const = 0;
        virtual void SetRoot(std::shared_ptr<SlokedCairoScreenComponent>) = 0;
        virtual void Close() = 0;
    };

    template <typename T>
    class SlokedCairoGraphicalWindow : public SlokedAbstractGraphicalWindow<T> {
     public:
        SlokedCairoGraphicalWindow(std::unique_ptr<SlokedAbstractCairoWindow> window, std::shared_ptr<T> component)
            : window(std::move(window)), component(std::move(component)) {}

        T &GetComponent() final {
            return *this->component;
        }

        const std::string &GetTitle() const final {
            return this->title;
        }

        void SetTitle(const std::string &) final {}

        SlokedGraphicsDimensions GetSize() const final {
            return this->window->GetSize();
        }

        bool Resize(SlokedGraphicsDimensions dim) final {
            this->window->SetSize(std::move(dim));
            return true;
        }

        void Close() final {
            this->window->Close();
        }

     private:
        std::unique_ptr<SlokedAbstractCairoWindow> window;
        std::shared_ptr<T> component;
        std::string title{""};
    };
}

#endif