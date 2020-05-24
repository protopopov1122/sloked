#ifndef SLOKED_SCREEN_GRAPHEMESTRING_H_
#define SLOKED_SCREEN_GRAPHEMESTRING_H_

#include "sloked/core/Grapheme.h"
#include "sloked/core/CharPreset.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/Character.h"

namespace sloked {

    struct GraphemeBounds {
        struct Position {
            TextPosition::Column offset;
            TextPosition::Column length;
        };

        template <typename Iter>
        static Iter FindByDirectOffset(const Iter &begin, const Iter &end,
                                       TextPosition::Column directOffset) {
            return std::find_if(
                begin, end, [directOffset](const auto &grapheme) {
                    return directOffset >= grapheme.directPosition.offset &&
                           directOffset < grapheme.directPosition.offset +
                                              grapheme.directPosition.length;
                });
        }

        template <typename OutIter>
        static void Split(std::string_view in, const Encoding &encoding,
                          const SlokedCharPreset &charPreset,
                          const SlokedGraphemeEnumerator &graphemes,
                          const SlokedFontProperties &fontProperties,
                          OutIter out) {
            TextPosition::Column column{0};
            TextPosition::Column codepointOffset{0};
            graphemes.Iterate(
                encoding, in, [&](auto start, auto length, auto codepoints) {
                    if (codepoints.Size() == 1 && codepoints[0] == U'\t') {
                        auto tabWidth = charPreset.GetCharWidth(U'\t', column);
                        const char32_t Space{U' '};
                        while (tabWidth-- > 0) {
                            *out++ = GraphemeBounds{
                                column, {codepointOffset,
                                static_cast<TextPosition::Column>(
                                    codepoints.Size())},
                                fontProperties.GetWidth(SlokedSpan(&Space, 1))};
                            column++;
                        }
                    } else {
                        *out++ =
                            GraphemeBounds{column, {codepointOffset,
                                           static_cast<TextPosition::Column>(
                                               codepoints.Size())},
                                           fontProperties.GetWidth(codepoints)};
                        column++;
                    }
                    codepointOffset += codepoints.Size();
                    return true;
                });
        }

        template <typename Iter>
        static auto TotalGraphicalWidth(const Iter &begin, const Iter &end) {
            SlokedGraphicsPoint::Coordinate total{0};
            std::for_each(begin, end, [&total](const auto &grapheme) {
                total += grapheme.graphicalWidth;
            });
            return total;
        }

        TextPosition::Column virtualOffset;
        Position directPosition;
        SlokedGraphicsPoint::Coordinate graphicalWidth;
    };

    class SlokedGraphemeStringLayout {
     public:
        using ConstIterator = std::vector<GraphemeBounds>::const_iterator;

        SlokedGraphemeStringLayout(std::string_view, const Encoding &,
                          const SlokedCharPreset &,
                          const SlokedGraphemeEnumerator &,
                          const SlokedFontProperties &);

        std::size_t Count() const;
        const GraphemeBounds &At(TextPosition::Column) const;
        ConstIterator FindByDirectOffset(TextPosition::Column) const;
        SlokedGraphicsPoint::Coordinate TotalGraphicalWidth(const ConstIterator &, const ConstIterator &) const;
        ConstIterator begin() const;
        ConstIterator end() const;

     private:
        std::vector<GraphemeBounds> layout;
    };
}

#endif
