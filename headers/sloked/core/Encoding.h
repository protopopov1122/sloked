#ifndef SLOKED_CORE_ENCODING_H_
#define SLOKED_CORE_ENCODING_H_

#include "sloked/Base.h"
#include <functional>
#include <string>
#include <memory>

namespace sloked {

    class Encoding {
     public:
        Encoding(const Encoding &) = delete;
        Encoding(Encoding &&) = delete;
        virtual ~Encoding() = default;
        Encoding &operator=(const Encoding &) = delete;
        Encoding &operator=(Encoding &&) = delete;

        virtual std::size_t CodepointCount(std::string_view) const = 0;
        virtual std::pair<std::size_t, std::size_t> GetCodepoint(std::string_view, std::size_t) const = 0;
        virtual bool IterateCodepoints(std::string_view, std::function<bool(std::size_t, std::size_t, char32_t)>) const = 0;
        virtual std::string Encode(char32_t) const = 0;
        virtual std::string Encode(std::u32string_view) const = 0;
        virtual std::u32string Decode(std::string_view) const;

        static const Encoding &Utf8;
        static const Encoding &Utf32LE;

     protected:
        Encoding() = default;
    };

    class EncodingConverter {
     public:
        EncodingConverter(const Encoding &, const Encoding &);
        std::string Convert(std::string_view) const;
        std::string ReverseConvert(std::string_view) const;
        
     private:
        const Encoding &from;
        const Encoding &to;
    };
}

#endif