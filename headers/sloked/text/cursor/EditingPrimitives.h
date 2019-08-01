#ifndef SLOKED_TEXT_CURSOR_EDITINGPRIMITIVES_H_
#define SLOKED_TEXT_CURSOR_EDITINGPRIMITIVES_H_

#include "sloked/core/Position.h"
#include "sloked/core/Encoding.h"
#include "sloked/text/TextBlock.h"

namespace sloked {

    class SlokedEditingPrimitives {
     public:
        static TextPosition Insert(TextBlock &, const Encoding &, const TextPosition &, std::string_view);
        static TextPosition Newline(TextBlock &, const Encoding &, const TextPosition &, std::string_view);
        static TextPosition DeleteBackward(TextBlock &, const Encoding &, const TextPosition &);
        static TextPosition DeleteForward(TextBlock &, const Encoding &, const TextPosition &);
        static TextPosition ClearRegion(TextBlock &, const Encoding &, const TextPosition &, const TextPosition &);
        static std::size_t GetOffset(std::string_view, TextPosition::Column, const Encoding &);
        static std::vector<std::string> Read(const TextBlock &, const Encoding &, const TextPosition &, const TextPosition &);
    };
}

#endif