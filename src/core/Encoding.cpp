#include "sloked/core/Encoding.h"
#include <unicode/unistr.h>
#include <unicode/brkiter.h>

namespace sloked {

    class Utf8Encoding : public Encoding {
     public:
        std::size_t CodepointCount(const std::string &str) const override {
            auto ustr = icu::UnicodeString::fromUTF8(str);
            return ustr.countChar32();
        }

        std::pair<std::size_t, std::size_t> GetCodepoint(const std::string &str, std::size_t idx) const override {
            auto ustr = icu::UnicodeString::fromUTF8(str);
            if (idx < static_cast<std::size_t>(ustr.countChar32())) {
                icu::UnicodeString u1(ustr, 0, idx);
                icu::UnicodeString u2(ustr, 0, idx + 1);
                std::string buffer;
                u1.toUTF8String(buffer);
                std::size_t start = buffer.size();
                buffer.clear();
                u2.toUTF8String(buffer);
                std::size_t end = buffer.size();
                return std::make_pair(start, end - start);
            } else {
                return std::make_pair(0, 0);
            }
        }

        bool IterateCodepoints(const std::string &str, std::function<bool(std::size_t, std::size_t, char32_t)> callback) const override {
            auto ustr = icu::UnicodeString::fromUTF8(str);
            std::size_t prev = 0;
            for (auto it = 0; it <= ustr.countChar32(); it++) {
                if (prev == static_cast<std::size_t>(it)) {
                    prev = it;
                    continue;
                }
                std::string buffer;
                icu::UnicodeString(ustr, 0, prev).toUTF8String(buffer);
                std::size_t start = buffer.size();
                buffer.clear();
                icu::UnicodeString(ustr, 0, it).toUTF8String(buffer);
                std::size_t end = buffer.size();
                if (!callback(start, end - start, ustr.char32At(prev))) {
                    return false;
                }
                prev = it;
            }
            return true;
        }

        std::string Encode(char32_t chr) const {
            icu::UnicodeString ustr(static_cast<UChar32>(chr));
            std::string str;
            ustr.toUTF8String(str);
            return str;
        }
    };

    static Utf8Encoding utf8Encoding;

    const Encoding &Encoding::Utf8 = utf8Encoding;
}