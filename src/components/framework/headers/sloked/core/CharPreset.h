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

#ifndef SLOKED_CORE_CHARPRESET_H_
#define SLOKED_CORE_CHARPRESET_H_

#include <cinttypes>

#include "sloked/core/Encoding.h"
#include "sloked/core/Event.h"
#include "sloked/core/Position.h"

namespace sloked {

    class SlokedCharPreset {
     public:
        using Listener = SlokedEventEmitter<const SlokedCharPreset &>::Listener;
        using Unbind = SlokedEventEmitter<const SlokedCharPreset &>::Unbind;

        virtual ~SlokedCharPreset() = default;
        virtual TextPosition::Column GetCharWidth(char32_t, TextPosition::Column) const = 0;
        virtual std::pair<std::size_t, std::size_t> GetRealPosition(
            std::string_view, TextPosition::Column, const Encoding &) const = 0;
        virtual Unbind OnChange(Listener) const = 0;

        static std::string EncodeTab(const SlokedCharPreset &, const Encoding &, TextPosition::Column);
    };

    class SlokedChangeableCharPreset : public SlokedCharPreset {
     public:
        virtual void SetTabWidth(TextPosition::Column) = 0;
    };

    class SlokedFixedWidthCharPreset : public SlokedChangeableCharPreset {
     public:
        SlokedFixedWidthCharPreset(TextPosition::Column);
        TextPosition::Column GetCharWidth(char32_t, TextPosition::Column) const final;
        std::pair<std::size_t, std::size_t> GetRealPosition(
            std::string_view, TextPosition::Column, const Encoding &) const final;
        Unbind OnChange(Listener) const final;

        void SetTabWidth(TextPosition::Column) final;

     private:
        TextPosition::Column tab_width;
        mutable SlokedEventEmitter<const SlokedCharPreset &> events;
    };

    class SlokedFixedWidthCharPresetProxy : public SlokedChangeableCharPreset {
        SlokedFixedWidthCharPresetProxy(const SlokedCharPreset &);
        ~SlokedFixedWidthCharPresetProxy();
        TextPosition::Column GetCharWidth(char32_t, TextPosition::Column) const final;
        std::pair<std::size_t, std::size_t> GetRealPosition(
            std::string_view, TextPosition::Column, const Encoding &) const final;
        Unbind OnChange(Listener) const final;

        void SetTabWidth(TextPosition::Column) final;
        void ResetTabWidth();

     private:
        const SlokedCharPreset &charPreset;
        std::optional<TextPosition::Column> tabWidth;
        Unbind unbindListener;
        mutable SlokedEventEmitter<const SlokedCharPreset &> events;
    };
}  // namespace sloked

#endif