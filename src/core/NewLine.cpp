#include "sloked/core/NewLine.h"
#include <algorithm>
#include <iostream>

namespace sloked {

    class AnsiLFNewLine : public NewLine {
     public:
        AnsiLFNewLine();
        void Iterate(std::string_view, Iterator) const override;
        std::size_t Count(std::string_view) const override;
    };

    class AnsiCRLFNewLine : public NewLine {
     public:
        AnsiCRLFNewLine();
        void Iterate(std::string_view, Iterator) const override;
        std::size_t Count(std::string_view) const override;
    };

    static AnsiLFNewLine AnsiLF_NewLine;
    static AnsiCRLFNewLine AnsiCRLF_NewLine;

    const NewLine &NewLine::AnsiLF = AnsiLF_NewLine;
    const NewLine &NewLine::AnsiCRLF = AnsiCRLF_NewLine;

    NewLine::NewLine(const std::string &symbol)
        : Width(symbol.size()), symbol(symbol) {}

    std::ostream &operator<<(std::ostream &os, const NewLine &newline) {
        return os << newline.symbol;
    }

    AnsiLFNewLine::AnsiLFNewLine()
        : NewLine("\n") {}

    void AnsiLFNewLine::Iterate(std::string_view str, Iterator iter) const {
        for (std::size_t i = 0; i < str.size(); i++) {
            if (str[i] == '\n') {
                iter(i);
            }
        }
    }

    std::size_t AnsiLFNewLine::Count(std::string_view str) const {
        return std::count(str.begin(), str.end(), '\n') + 1;
    }

    AnsiCRLFNewLine::AnsiCRLFNewLine()
        : NewLine("\r\n") {}

    void AnsiCRLFNewLine::Iterate(std::string_view str, Iterator iter) const {
        for (std::size_t i = 0; i < str.size(); i++) {
            if (i + 1 < str.size() && str[i] == '\r' && str[i + 1] == '\n') {
                iter(i);
            }
        }
    }

    std::size_t AnsiCRLFNewLine::Count(std::string_view str) const {
        std::size_t line = 1;
        for (std::size_t i = 0; i < str.size(); i++) {
            if (i + 1 < str.size() && str[i] == '\r' && str[i + 1] == '\n') {
                line++;
            }
        }
        return line;
    }
}