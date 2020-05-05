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

#include "sloked/core/CharPreset.h"

namespace sloked {

    std::string SlokedCharPreset::EncodeTab(const SlokedCharPreset &preset,
                                            const Encoding &encoding,
                                            TextPosition::Column column) {
        auto tab_width = preset.GetCharWidth(U'\t', column);
        std::u32string tab(tab_width, U' ');
        return encoding.Encode(tab);
    }

    SlokedFixedWidthCharPreset::SlokedFixedWidthCharPreset(
        TextPosition::Column tab_width)
        : tab_width(tab_width) {}

    TextPosition::Column SlokedFixedWidthCharPreset::GetCharWidth(
        char32_t chr, TextPosition::Column position) const {
        if (chr != '\t') {
            return 1;
        } else {
            return this->tab_width - position % this->tab_width;
        }
    }

    std::pair<std::size_t, std::size_t>
        SlokedFixedWidthCharPreset::GetRealPosition(
            std::string_view str, TextPosition::Column idx,
            const Encoding &encoding) const {
        std::pair<std::size_t, std::size_t> res{0, 0};
        encoding.IterateCodepoints(
            str, [&](auto start, auto length, auto value) {
                res.first = res.second;
                res.second += GetCharWidth(value, res.first);
                return idx--;
            });
        return res;
    }

    SlokedCharPreset::Unbind SlokedFixedWidthCharPreset::OnChange(
        Listener listener) const {
        return this->events.Listen(std::move(listener));
    }

    void SlokedFixedWidthCharPreset::SetTabWidth(TextPosition::Column width) {
        this->tab_width = width;
        this->events.Emit(*this);
    }

    SlokedFixedWidthCharPresetProxy::SlokedFixedWidthCharPresetProxy(
        const SlokedCharPreset &charPreset)
        : charPreset(charPreset), tabWidth{}, unbindListener{nullptr} {
        this->unbindListener = this->charPreset.OnChange([this](const auto &) {
            this->events.Emit(*this);
        });
    }

    SlokedFixedWidthCharPresetProxy::~SlokedFixedWidthCharPresetProxy() {
        if (this->unbindListener) {
            this->unbindListener();
        }
    }

    TextPosition::Column SlokedFixedWidthCharPresetProxy::GetCharWidth(
        char32_t chr, TextPosition::Column column) const {
        if (chr != '\t' || !this->tabWidth.has_value()) {
            return 1;
        } else {
            return this->tabWidth.value() - column % this->tabWidth.value();
        }
    }
    std::pair<std::size_t, std::size_t>
        SlokedFixedWidthCharPresetProxy::GetRealPosition(
            std::string_view str, TextPosition::Column idx,
            const Encoding &encoding) const {
        std::pair<std::size_t, std::size_t> res{0, 0};
        encoding.IterateCodepoints(
            str, [&](auto start, auto length, auto value) {
                res.first = res.second;
                res.second += GetCharWidth(value, res.first);
                return idx--;
            });
        return res;
    }

    SlokedFixedWidthCharPresetProxy::Unbind SlokedFixedWidthCharPresetProxy::OnChange(Listener listener) const {
        return this->events.Listen(std::move(listener));
    }

    void SlokedFixedWidthCharPresetProxy::SetTabWidth(TextPosition::Column width) {
        this->tabWidth = width;
        this->events.Emit(*this);   
    }

    void SlokedFixedWidthCharPresetProxy::ResetTabWidth() {
        this->tabWidth.reset();
        this->events.Emit(*this);
    }
}  // namespace sloked