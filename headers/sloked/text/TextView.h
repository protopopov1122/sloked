#ifndef SLOKED_TEXT_TEXTVIEW_H_
#define SLOKED_TEXT_TEXTVIEW_H_

#include "sloked/core/NewLine.h"
#include "sloked/text/TextBlock.h"

namespace sloked {

    class TextView {
     public:
        static std::unique_ptr<TextBlock> Open(std::string_view, const NewLine &, const TextBlockFactory &);
    };
}

#endif