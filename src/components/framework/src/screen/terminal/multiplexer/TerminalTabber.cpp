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

#include "sloked/screen/terminal/multiplexer/TerminalTabber.h"

#include <functional>

#include "sloked/core/Error.h"

namespace sloked {

    class TerminalTab : public SlokedTerminal {
     public:
        TerminalTab(std::function<bool(const TerminalTab *)> is_current,
                    SlokedTerminal &term)
            : is_current(is_current), term(term) {}

        void SetPosition(Line l, Column c) override {
            if (this->is_current(this)) {
                this->term.SetPosition(l, c);
            }
        }

        void MoveUp(Line l) override {
            if (this->is_current(this)) {
                this->term.MoveUp(l);
            }
        }

        void MoveDown(Line l) override {
            if (this->is_current(this)) {
                this->term.MoveDown(l);
            }
        }

        void MoveBackward(Column c) override {
            if (this->is_current(this)) {
                this->term.MoveBackward(c);
            }
        }

        void MoveForward(Column c) override {
            if (this->is_current(this)) {
                this->term.MoveForward(c);
            }
        }

        void ShowCursor(bool s) override {
            if (this->is_current(this)) {
                this->term.ShowCursor(s);
            }
        }

        void ClearScreen() override {
            if (this->is_current(this)) {
                this->term.ClearScreen();
            }
        }

        void ClearChars(Column c) override {
            if (this->is_current(this)) {
                this->term.ClearChars(c);
            }
        }

        Column GetWidth() override {
            return this->term.GetWidth();
        }

        Line GetHeight() override {
            return this->term.GetHeight();
        }

        void Write(std::string_view str) override {
            if (this->is_current(this)) {
                this->term.Write(str);
            }
        }

        void SetGraphicsMode(SlokedTextGraphics m) override {
            if (this->is_current(this)) {
                this->term.SetGraphicsMode(m);
            }
        }

        void SetGraphicsMode(SlokedBackgroundGraphics m) override {
            if (this->is_current(this)) {
                this->term.SetGraphicsMode(m);
            }
        }

        void SetGraphicsMode(SlokedForegroundGraphics m) override {
            if (this->is_current(this)) {
                this->term.SetGraphicsMode(m);
            }
        }

        bool UpdateDimensions() override {
            return this->term.UpdateDimensions();
        }

        void Flush(bool f) override {
            this->term.Flush(f);
        }

     private:
        std::function<bool(const TerminalTab *)> is_current;
        SlokedTerminal &term;
    };

    TerminalTabber::TerminalTabber(SlokedTerminal &term)
        : term(term), current_tab(0) {}

    TerminalTabber::TabId TerminalTabber::GetTabCount() const {
        return this->tabs.size();
    }

    std::optional<TerminalTabber::TabId> TerminalTabber::GetCurrentTab() const {
        if (this->current_tab < this->tabs.size()) {
            return this->current_tab;
        } else {
            return std::optional<TabId>{};
        }
    }

    SlokedTerminal &TerminalTabber::GetTab(TabId idx) const {
        if (idx < this->tabs.size()) {
            return *this->tabs.at(idx);
        } else {
            throw SlokedError("Tab #" + std::to_string(idx) + " not found");
        }
    }

    TextPosition TerminalTabber::GetDimensions() {
        return {this->term.GetHeight(), this->term.GetWidth()};
    }

    SlokedIndexed<SlokedTerminal &, TerminalTabber::TabId>
        TerminalTabber::NewTab() {
        auto tab = std::make_shared<TerminalTab>(
            [this](auto tabPtr) {
                if (this->current_tab < this->tabs.size()) {
                    return tabPtr == this->tabs.at(this->current_tab).get();
                } else {
                    return false;
                }
            },
            this->term);
        this->tabs.push_back(tab);
        return {this->tabs.size() - 1, *tab};
    }

    SlokedIndexed<SlokedTerminal &, TerminalTabber::TabId>
        TerminalTabber::NewTab(TabId idx) {
        if (idx > this->tabs.size()) {
            throw SlokedError("Invalid tab index " + std::to_string(idx));
        }
        auto tab = std::make_shared<TerminalTab>(
            [this](auto tabPtr) {
                if (this->current_tab < this->tabs.size()) {
                    return tabPtr == this->tabs.at(this->current_tab).get();
                } else {
                    return false;
                }
            },
            this->term);
        this->tabs.insert(this->tabs.begin() + idx, tab);
        return {idx, *tab};
    }

    bool TerminalTabber::SelectTab(TabId tab) {
        if (tab <= this->tabs.size()) {
            this->current_tab = tab;
            return true;
        } else {
            return false;
        }
    }

    bool TerminalTabber::MoveTab(TabId src, TabId dst) {
        if (src < this->tabs.size() && dst < this->tabs.size()) {
            if (src < dst) {
                this->tabs.insert(this->tabs.begin() + dst + 1,
                                  this->tabs.at(src));
                this->tabs.erase(this->tabs.begin() + src);
            } else if (src > dst) {
                this->tabs.insert(this->tabs.begin() + dst, this->tabs.at(src));
                this->tabs.erase(this->tabs.begin() + src + 1);
            }
            return true;
        } else {
            return false;
        }
    }

    bool TerminalTabber::CloseTab(TabId tab) {
        if (tab < this->tabs.size()) {
            this->tabs.erase(this->tabs.begin() + tab);
            return true;
        } else {
            return false;
        }
    }
}  // namespace sloked