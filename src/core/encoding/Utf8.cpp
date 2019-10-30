/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

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

#include "sloked/core/Encoding.h"
#include "sloked/core/Error.h"

namespace sloked {

    static size_t Char32ToUtf8(char *const buffer, const char32_t code) {
        if (code <= 0x7F) {
            buffer[0] = code;
            return 1;
        } else if (code <= 0x7FF) {
            buffer[0] = 0xC0 | (code >> 6);
            buffer[1] = 0x80 | (code & 0x3F);
            return 2;
        } else if (code <= 0xFFFF) {
            buffer[0] = 0xE0 | (code >> 12);
            buffer[1] = 0x80 | ((code >> 6) & 0x3F);
            buffer[2] = 0x80 | (code & 0x3F);
            return 3;
        } else if (code <= 0x10FFFF) {
            buffer[0] = 0xF0 | (code >> 18);
            buffer[1] = 0x80 | ((code >> 12) & 0x3F);
            buffer[2] = 0x80 | ((code >> 6) & 0x3F);
            buffer[3] = 0x80 | (code & 0x3F);
            return 4;
        } else {
            throw SlokedError("UTF-8: Invalid codepoint");
        }
    }

    class Utf8Encoding : public Encoding {
     public:
        const std::string &GetIdentifier() const override {
            return EncodingIdentifiers::Utf8;
        }

        std::size_t CodepointCount(std::string_view str) const override {
            std::size_t count = 0;
            for (std::size_t i = 0; i < str.size(); i++) {
                char current = str[i];
                count++;
                if ((current & 0xc0) != 0xc0) {
                } else if ((current & 0xe0) != 0xe0) {
                    i++;
                } else if ((current & 0xf0) != 0xf0) {
                    i += 2;
                } else {
                    i += 3;
                }
            }
            return count;
        }

        std::pair<std::size_t, std::size_t> GetCodepoint(std::string_view str, std::size_t idx) const override {
#define ASSERT_WIDTH(x) do { \
                if (i + (x) - 1 >= str.size()) { \
                    return std::make_pair(0, 0); \
                } \
            } while (false)
            for (std::size_t i = 0; i < str.size();) {
                char current = str[i];
                std::size_t width = 0;
                if ((current & 0xc0) != 0xc0){
                    width = 1;
                    ASSERT_WIDTH(width);
                } else if ((current & 0xe0) != 0xe0) {
                    width = 2;
                    ASSERT_WIDTH(width);
                } else if ((current & 0xf0) != 0xf0) {
                    width = 3;
                    ASSERT_WIDTH(width);
                } else {
                    width = 4;
                    ASSERT_WIDTH(width);
                }
                if (!idx--) {
                    return std::make_pair(i, width);
                } else {
                    i += width;
                }
            }
#undef ASSERT_WIDTH
            return std::make_pair(0, 0);
        }

        std::optional<std::size_t> GetCodepointByOffset(std::string_view str, std::size_t symbol_offset) const override {
            if (symbol_offset >= str.size()) {
                return {};
            }
            std::optional<std::size_t> value;
            std::size_t idx = 0;
            this->IterateCodepoints(str, [&](std::size_t offset, std::size_t length, char32_t chr) {
                if (offset > symbol_offset) {
                    value = idx - 1;
                    return false;
                } else if (offset == symbol_offset) {
                    value = idx;
                    return false;
                }
                idx++;
                return true;
            });
            return value;
        }

        bool IterateCodepoints(std::string_view str, std::function<bool(std::size_t, std::size_t, char32_t)> callback) const override {
            bool res = true;
            const std::size_t string_len = str.size();
#define ASSERT_WIDTH(x) do { \
                            if (i + (x) - 1 >= string_len) { \
                                return false; \
                            } \
                        } while (false)
            for (std::size_t i = 0; i < string_len && res;) {
                char32_t result;
                std::size_t width = 0;
                char current = str[i];
                if ((current & 0xc0) != 0xc0){
                    width = 1;
                    ASSERT_WIDTH(width);
                    result = current;
                } else if ((current & 0xe0) != 0xe0) {
                    width = 2;
                    ASSERT_WIDTH(width);
                    result = ((current & 0x1f) << 6)
                        | (str[i + 1] & 0x3f);
                } else if ((current & 0xf0) != 0xf0) {
                    width = 3;
                    ASSERT_WIDTH(width);
                    result = ((current & 0xf) << 12)
                        | ((str[i + 1] & 0x3f) << 6)
                        | (str[i + 2] & 0x3f);
                } else {
                    width = 4;
                    ASSERT_WIDTH(width);
                    result = ((current & 0x7) << 18)
                        | ((str[i + 1] & 0x3f) << 12)
                        | ((str[i + 2] & 0x3f) << 6)
                        | (str[i + 3] & 0x3f);
                }
                res = callback(i, width, result);
                i += width;
            }
#undef ASSERT_WIDTH
            return res;
        }

        std::string Encode(char32_t chr) const {
            char buffer[5] {0};
            Char32ToUtf8(buffer, chr);
            return std::string(buffer);
        }

        static char *EncodeImpl(char *buffer, std::u32string_view u32str) {
            for (char32_t u32chr : u32str) {
                buffer += Char32ToUtf8(buffer, u32chr);
            }
            return buffer;
        }

        std::string Encode(std::u32string_view u32str) const override {
            constexpr std::size_t MaxStaticBuffer = 16;
            if (u32str.size() < MaxStaticBuffer){
                char buffer[MaxStaticBuffer * 4];
                return std::string(buffer, EncodeImpl(buffer, u32str) - buffer);
            } else {
                std::unique_ptr<char[]> buffer(new char[u32str.size() * 4]);
                return std::string(buffer.get(), EncodeImpl(buffer.get(), u32str) - buffer.get());
            }
        }
    };

    static Utf8Encoding utf8Encoding;
    const Encoding &Encoding::Utf8 = utf8Encoding;
}