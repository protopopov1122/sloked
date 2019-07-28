#ifndef SLOKED_CORE_NEWLINE_H_
#define SLOKED_CORE_NEWLINE_H_

#include "sloked/Base.h"
#include "sloked/core/Encoding.h"
#include <string>
#include <functional>
#include <memory>
#include <iosfwd>

namespace sloked {

    class NewLine {
     public:
        using Iterator = std::function<void(std::size_t, std::size_t)>;

        NewLine(const std::string &);
        NewLine(const NewLine &) = delete;
        NewLine(NewLine &&) = delete;
        virtual ~NewLine() = default;

        NewLine &operator=(const NewLine &) = delete;
        NewLine &operator=(NewLine &&) = delete;

        virtual void Iterate(std::string_view, Iterator) const = 0;
        virtual std::size_t Count(std::string_view) const = 0;

        const std::size_t Width;
        static std::unique_ptr<NewLine> LF(const Encoding & = Encoding::Utf8);
        static std::unique_ptr<NewLine> CRLF(const Encoding & = Encoding::Utf8);
    
     private:
        friend std::ostream &operator<<(std::ostream &, const NewLine &);
        std::string symbol;
    };
}

#endif