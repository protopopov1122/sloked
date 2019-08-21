#ifndef SLOKED_TEXT_TEXTFRAME_H_
#define SLOKED_TEXT_TEXTFRAME_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/Position.h"
#include "sloked/core/CharWidth.h"
#include "sloked/text/TextBlock.h"
#include <vector>

namespace sloked {

    class TextFrameView : public TextBlockView {
     public:
        TextFrameView(const TextBlockView &, const Encoding &, const SlokedCharWidth &);

        std::size_t GetLastLine() const override;
        std::size_t GetTotalLength() const override;
        std::string_view GetLine(std::size_t) const override;
        bool Empty() const override;

        void Update(const TextPosition &, const TextPosition &);
        const TextPosition &GetOffset() const;
        const TextPosition &GetSize() const;

     protected:
        std::ostream &dump(std::ostream &) const override;

     private:
        void VisitLines(std::function<void(std::size_t, std::string_view)>) const;
        std::string PreprocessLine(std::string_view) const;

        const TextBlockView &text;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        TextPosition offset;
        TextPosition size;
        mutable std::vector<std::string> buffer;
    };
}

#endif