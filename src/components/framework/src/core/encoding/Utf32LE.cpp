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

#include "sloked/core/Encoding.h"
#include "sloked/core/Error.h"

namespace sloked {

    class Utf32LEEncoding : public Encoding {
     public:
        const std::string &GetIdentifier() const override {
            return EncodingIdentifiers::Utf32LE;
        }

        std::size_t CodepointCount(std::string_view view) const override {
            if (view.size() % 4 == 0) {
                return view.size() / 4;
            } else {
                throw SlokedError("UTF-32LE: Invalid length");
            }
        }

        std::optional<Codepoint> GetCodepoint(
            std::string_view view, std::size_t idx) const override {
            bool validView = view.size() % 4 == 0;
            if (validView && idx * 4 < view.size()) {
                return Codepoint {
                    idx * 4, 4,
                    static_cast<char32_t>(view[4 * idx] | (view[4 * idx + 1] << 8) |
                               (view[4 * idx + 2] << 16) |
                               (view[4 * idx + 3] << 24))
                };
            } else if (validView) {
                return {};
            } else {
                throw SlokedError("UTF-32LE: Invalid length");
            }
        }

        std::optional<std::size_t> GetCodepointByOffset(
            std::string_view str, std::size_t symbol_offset) const override {
            if (symbol_offset >= str.size()) {
                return {};
            }
            return symbol_offset / 4;
        }

        bool IterateCodepoints(
            std::string_view view,
            std::function<bool(std::size_t, std::size_t, char32_t)> iter)
            const override {
            if (view.size() % 4 != 0) {
                throw SlokedError("UTF-32LE: Invalid length");
            }
            bool res = true;
            for (std::size_t i = 0; i < view.size() / 4 && res; i++) {
                char32_t chr = view[4 * i] | (view[4 * i + 1] << 8) |
                               (view[4 * i + 2] << 16) |
                               (view[4 * i + 3] << 24);
                res = iter(i * 4, 4, chr);
            }
            return res;
        }

        bool Iterate(Iterator &iter, std::string_view string,
                     std::size_t length) const override {
            if (length % 4 != 0) {
                throw SlokedError("UTF-32LE: Invalid length");
            }
            if (iter.length > 0) {
                iter.start += iter.length;
            }
            if (iter.start == length) {
                return false;
            }

            iter.length = 4;
            iter.value = string[iter.start] | (string[iter.start + 1] << 8) |
                         (string[iter.start + 2] << 16) |
                         (string[iter.start + 3] << 24);
            return true;
        }

        std::string Encode(char32_t chr) const override {
            char buffer[] = {static_cast<char>(chr & 0xff),
                             static_cast<char>((chr >> 8) & 0xff),
                             static_cast<char>((chr >> 16) & 0xff),
                             static_cast<char>((chr >> 24) & 0xff)};
            return std::string(buffer, 4);
        }

        static char *EncodeImpl(char *buffer, std::u32string_view u32str) {
            for (std::size_t i = 0; i < u32str.size(); i++) {
                char32_t chr = u32str[i];
                *buffer++ = static_cast<char>(chr & 0xff);
                *buffer++ = static_cast<char>((chr >> 8) & 0xff);
                *buffer++ = static_cast<char>((chr >> 16) & 0xff);
                *buffer++ = static_cast<char>((chr >> 24) & 0xff);
            }
            return buffer;
        }

        std::string Encode(std::u32string_view u32str) const override {
            constexpr std::size_t MaxStaticBuffer = 16;
            if (u32str.size() < MaxStaticBuffer) {
                char buffer[MaxStaticBuffer * 4];
                return std::string(buffer, EncodeImpl(buffer, u32str) - buffer);
            } else {
                std::unique_ptr<char[]> buffer(new char[u32str.size() * 4]);
                return std::string(
                    buffer.get(),
                    EncodeImpl(buffer.get(), u32str) - buffer.get());
            }
        }
    };

    static Utf32LEEncoding utf32Encoding;
    const Encoding &Encoding::Utf32LE = utf32Encoding;
}  // namespace sloked