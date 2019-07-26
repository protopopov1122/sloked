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

        virtual std::size_t CodepointCount(const std::string &) const = 0;
        virtual std::pair<std::size_t, std::size_t> GetCodepoint(const std::string &, std::size_t) const = 0;
        virtual bool IterateCodepoints(const std::string &, std::function<bool(std::size_t, std::size_t)>) const = 0;

        static const Encoding &Utf8;

     protected:
        Encoding() = default;
    };
}

#endif