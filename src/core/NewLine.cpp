#include "sloked/core/NewLine.h"
#include <algorithm>
#include <iostream>

namespace sloked {

    class LFNewLine : public NewLine {
     public:
        LFNewLine();
        void Iterate(std::string_view, Iterator) const override;
        std::size_t Count(std::string_view) const override;
    };

    class CRLFNewLine : public NewLine {
     public:
        CRLFNewLine();
        void Iterate(std::string_view, Iterator) const override;
        std::size_t Count(std::string_view) const override;
    };

    static LFNewLine LF_NewLine;
    static CRLFNewLine CRLF_NewLine;

    const NewLine &NewLine::LF = LF_NewLine;
    const NewLine &NewLine::CRLF = CRLF_NewLine;

    NewLine::NewLine(const std::string &symbol)
        : Width(symbol.size()), symbol(symbol) {}

    std::ostream &operator<<(std::ostream &os, const NewLine &newline) {
        return os << newline.symbol;
    }

    LFNewLine::LFNewLine()
        : NewLine("\n") {}

    void LFNewLine::Iterate(std::string_view str, Iterator iter) const {
        for (std::size_t i = 0; i < str.size(); i++) {
            if (str[i] == '\n') {
                iter(i);
            }
        }
    }

    std::size_t LFNewLine::Count(std::string_view str) const {
        return std::count(str.begin(), str.end(), '\n') + 1;
    }

    CRLFNewLine::CRLFNewLine()
        : NewLine("\r\n") {}

    void CRLFNewLine::Iterate(std::string_view str, Iterator iter) const {
        for (std::size_t i = 0; i < str.size(); i++) {
            if (i + 1 < str.size() && str[i] == '\r' && str[i + 1] == '\n') {
                iter(i);
            }
        }
    }

    std::size_t CRLFNewLine::Count(std::string_view str) const {
        std::size_t line = 1;
        for (std::size_t i = 0; i < str.size(); i++) {
            if (i + 1 < str.size() && str[i] == '\r' && str[i + 1] == '\n') {
                line++;
            }
        }
        return line;
    }
}