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

#include "sloked/bootstrap/Graphics.h"

#include "sloked/compat/screen/graphics/Compat.h"
#include "sloked/compat/screen/terminal/Compat.h"
#include "sloked/compat/screen/terminal/TerminalResize.h"
#include "sloked/core/Error.h"
#include "sloked/editor/terminal/ScreenProvider.h"
#include "sloked/screen/terminal/TerminalSize.h"
#include "sloked/screen/terminal/multiplexer/TerminalBuffer.h"
#include "sloked/screen/terminal/NullTerminal.h"

namespace sloked {

    class SlokedGraphicalBootstrapScreen : public SlokedScreenProvider {
     public:
        struct GUI {
            GUI(int width, int height)
                : gui(SlokedGraphicsCompat::GetGraphics(this->screenMgr)),
                  terminal(
                      gui->OpenTerminal({{width, height}, "Monospace 10"})) {
                this->screenMgr.Start(std::chrono::milliseconds(50));
            }

            ~GUI() {
                this->screenMgr.Stop();
                this->terminal->Close();
            }

            SlokedGraphicalTerminal &GetTerminal() {
                return this->terminal->GetTerminal();
            }

            SlokedScreenManager screenMgr;
            std::unique_ptr<SlokedGraphicalComponents> gui;
            std::unique_ptr<SlokedGraphicalTerminalWindow> terminal;
        };

        SlokedGraphicalBootstrapScreen(std::unique_ptr<SlokedCharPreset> charPreset)
            : gui(1024, 960),
              console(gui.GetTerminal(), gui.GetTerminal().GetEncoding(),
                      *charPreset),
              provider(console, gui.GetTerminal().GetEncoding(), *charPreset,
                       gui.GetTerminal(), gui.GetTerminal().GetTerminalSize()),
              charPreset(std::move(charPreset)) {
        }

        void Render(std::function<void(SlokedScreenComponent &)> fn) final {
            this->provider.Render(std::move(fn));
        }

        std::vector<SlokedKeyboardInput> ReceiveInput(
            std::chrono::system_clock::duration timeout) final {
            return this->provider.ReceiveInput(timeout);
        }

        SlokedMonitor<SlokedScreenComponent &> &GetScreen() final {
            return this->provider.GetScreen();
        }

        SlokedScreenSize &GetSize() final {
            return this->provider.GetSize();
        }

        const Encoding &GetEncoding() final {
            return this->provider.GetEncoding();
        }

        const SlokedCharPreset &GetCharPreset() final {
            return this->provider.GetCharPreset();
        }

     private:
        GUI gui;
        BufferedTerminal console;
        SlokedTerminalScreenProvider provider;
        std::unique_ptr<SlokedCharPreset> charPreset;
    };

    class SlokedTerminalBootstrapScreen : public SlokedScreenProvider {
     public:
        SlokedTerminalBootstrapScreen(std::unique_ptr<SlokedCharPreset> charPreset)
            : terminal(SlokedTerminalCompat::GetSystemTerminal()),
              size(terminal,
                   [](auto callback) {
                       return SlokedTerminalResizeListener::Bind(callback);
                   }),
              console(terminal, Encoding::Get("system"), *charPreset),
              provider(console, Encoding::Get("system"), *charPreset, terminal,
                       size),
              charPreset(std::move(charPreset)) {}

        void Render(std::function<void(SlokedScreenComponent &)> fn) final {
            this->provider.Render(std::move(fn));
        }

        std::vector<SlokedKeyboardInput> ReceiveInput(
            std::chrono::system_clock::duration timeout) final {
            return this->provider.ReceiveInput(timeout);
        }

        SlokedMonitor<SlokedScreenComponent &> &GetScreen() final {
            return this->provider.GetScreen();
        }

        SlokedScreenSize &GetSize() final {
            return this->provider.GetSize();
        }

        const Encoding &GetEncoding() final {
            return this->provider.GetEncoding();
        }

        const SlokedCharPreset &GetCharPreset() final {
            return this->provider.GetCharPreset();
        }

     private:
        SlokedDuplexTerminal &terminal;
        SlokedTerminalSize<
            std::function<std::function<void()>(std::function<void()>)>>
            size;
        BufferedTerminal console;
        SlokedTerminalScreenProvider provider;
        std::unique_ptr<SlokedCharPreset> charPreset;
    };

    class SlokedNullBootstrapScreen : public SlokedScreenProvider {
     public:
        SlokedNullBootstrapScreen(std::unique_ptr<SlokedCharPreset> charPreset)
            : terminal(25, 80),
              size(terminal,
                   [](auto callback) {
                       return []{};
                   }),
              console(terminal, Encoding::Get("system"), *charPreset),
              provider(console, Encoding::Get("system"), *charPreset, terminal,
                       size),
              charPreset(std::move(charPreset)) {}

        void Render(std::function<void(SlokedScreenComponent &)> fn) final {
            this->provider.Render(std::move(fn));
        }

        std::vector<SlokedKeyboardInput> ReceiveInput(
            std::chrono::system_clock::duration timeout) final {
            return this->provider.ReceiveInput(timeout);
        }

        SlokedMonitor<SlokedScreenComponent &> &GetScreen() final {
            return this->provider.GetScreen();
        }

        SlokedScreenSize &GetSize() final {
            return this->provider.GetSize();
        }

        const Encoding &GetEncoding() final {
            return this->provider.GetEncoding();
        }

        const SlokedCharPreset &GetCharPreset() final {
            return this->provider.GetCharPreset();
        }

     private:
        SlokedNullTerminal terminal;
        SlokedTerminalSize<
            std::function<std::function<void()>(std::function<void()>)>>
            size;
        BufferedTerminal console;
        SlokedTerminalScreenProvider provider;
        std::unique_ptr<SlokedCharPreset> charPreset;
    };

    std::unique_ptr<SlokedScreenProvider> SlokedBootstrapScreenFactory::Make(
        const SlokedUri &uri, std::unique_ptr<SlokedCharPreset> charPreset) {
        if (uri.GetScheme() == "terminal") {
            return std::make_unique<SlokedTerminalBootstrapScreen>(std::move(charPreset));
        } else if (uri.GetScheme() == "null") {
            return std::make_unique<SlokedNullBootstrapScreen>(std::move(charPreset));
        } else if (uri.GetScheme() == "graphics") {
            const auto &query = uri.GetQuery();
            if constexpr (SlokedGraphicsCompat::HasGraphics()) {
                return std::make_unique<SlokedGraphicalBootstrapScreen>(
                    std::move(charPreset));
            } else if (query.has_value() && query.value().Has("fallback") &&
                       query.value().Get("fallback") == "terminal") {
                return std::make_unique<SlokedTerminalBootstrapScreen>(
                    std::move(charPreset));
            } else {
                throw SlokedError(
                    "Screen: Graphical screen unavailable without fallback");
            }
        } else {
            throw SlokedError("Screen: Unknown screen provider \'" +
                              uri.GetScheme() + "\'");
        }
    }
}  // namespace sloked