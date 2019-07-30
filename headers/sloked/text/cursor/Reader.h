#ifndef SLOKED_TEXT_CURSOR_READER_H_
#define SLOKED_TEXT_CURSOR_READER_H_

#include "sloked/Base.h"
#include "sloked/core/Position.h"
#include <vector>
#include <string>

namespace sloked {

    class SlokedTextReader {
     public:
        virtual ~SlokedTextReader() = default;

        virtual TextPosition::Column LineLength(TextPosition::Line) const = 0;
        virtual TextPosition::Line LineCount() const = 0;
        virtual std::vector<std::string> Read(const TextPosition &, const TextPosition &) const = 0;
    };
}

#endif