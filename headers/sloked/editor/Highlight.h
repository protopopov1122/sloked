#ifndef SLOKED_EDITOR_HIGHLIGHT_H_
#define SLOKED_EDITOR_HIGHLIGHT_H_

#include "sloked/text/TextFrame.h"
#include "sloked/text/fragment/TaggedText.h"

namespace sloked {

    template <typename T>
    class SlokedTaggedFrame {
     public:
        struct TaggedFragmentContent {
            std::string_view content;
            const TaggedTextFragment<T> *tag;
        };
        using TaggedLine = std::vector<TaggedFragmentContent>;

        SlokedTaggedFrame(const TextBlockView &text, const Encoding &encoding, const SlokedCharWidth &charWidth, SlokedTaggedText<T> &tags)
            : text(text, encoding, charWidth), tags(tags) {}

        void Update(const TextPosition &offset, const TextPosition &dim) {
            this->content.clear();
            this->text.Update(offset, dim);
            for (TextPosition::Line line = 0; !text.Empty() && line <= text.GetLastLine(); line++) {
                content.push_back(TaggedLine{});
                TaggedLine &currentLine = content.back();
                std::string_view currentContent = text.GetLine(line);
                // for (TextPosition::Column col = 0; c9o)w
            }
        }

     private:
        TextFrameView text;
        SlokedTaggedText<T> &tags;
        std::vector<TaggedLine> content;
    };
}

#endif
