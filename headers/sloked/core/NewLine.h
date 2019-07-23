#ifndef SLOKED_CORE_NEWLINE_H_
#define SLOKED_CORE_NEWLINE_H_

#include "sloked/Base.h"
#include <string>
#include <functional>
#include <iosfwd>

namespace sloked {

    class NewLine {
     public:
        using Iterator = std::function<void(std::size_t)>;

        NewLine(const std::string &);
        NewLine(const NewLine &) = delete;
        NewLine(NewLine &&) = delete;
        virtual ~NewLine() = default;

        NewLine &operator=(const NewLine &) = delete;
        NewLine &operator=(NewLine &&) = delete;

        virtual void Iterate(std::string_view, Iterator) const = 0;
        virtual std::size_t Count(std::string_view) const = 0;

        friend std::ostream &operator<<(std::ostream &, const NewLine &);

        const std::size_t Width;
        static const NewLine &LF;
        static const NewLine &CRLF;
    
     private:
        std::string symbol;
    };
}

#endif