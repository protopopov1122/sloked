/*
  SPDX-License-Identifier: LGPL-3.0-or-later

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_TEXT_TEXTCHUNK_H_
#define SLOKED_TEXT_TEXTCHUNK_H_

#include "sloked/core/AVL.h"
#include "sloked/core/RangeMap.h"
#include "sloked/core/NewLine.h"
#include "sloked/text/TextBlock.h"
#include <map>
#include <limits>
#include <optional>
#include <iosfwd>

namespace sloked {

    class TextChunk;

    class TextChunkFactory : public TextBlockFactory {
     public:
        TextChunkFactory(const NewLine &);
        std::unique_ptr<TextBlock> make(std::string_view) const override;
    
     private:
        const NewLine &newline;
    };

    class TextChunk : public TextBlockImpl<TextChunk>, public AVLNode<TextChunk> {
     public:
        TextChunk(const NewLine &, std::string_view, std::size_t = 0);
        TextChunk(const NewLine &, std::unique_ptr<TextChunk>, std::optional<std::string>, std::unique_ptr<TextChunk>, std::size_t = 0);

        std::size_t GetLastLine() const override;
        std::size_t GetTotalLength() const override;
        std::string_view GetLine(std::size_t) const override;
        bool Empty() const override;
        void Visit(std::size_t, std::size_t, Visitor) const override;
        
        void SetLine(std::size_t, std::string_view) override;
        void EraseLine(std::size_t) override;
        void InsertLine(std::size_t, std::string_view) override;
        void Optimize() override;

        void Squash();
        void Compact();
        void Balance();
        std::size_t GetHeight() const;

        friend std::ostream &operator<<(std::ostream &, const TextChunk &);

     protected:
        void AvlSwapContent(TextChunk &) override;
        void AvlUpdate() override;
        
     private:
        template <typename Line, typename Offset, typename Length = Offset>
        class Map;

        using DefaultMap = Map<std::size_t, std::size_t, std::size_t>;

        void Normalize();
        void Remap();
        void UpdateStats();

        const NewLine &newline;
        bool has_content;
        std::size_t height;
        std::size_t first_line;
        std::optional<std::string> content;
        std::unique_ptr<DefaultMap> chunk_map;
    };

    template <typename Line, typename Offset, typename Length = Offset>
    class TextChunk::Map {
     public:
        Map(const std::optional<std::string> &content, TextChunk *begin, TextChunk *end, const NewLine & newline)
            : lines(0), last_line(0), total_length(0) {
            if (begin && !begin->Empty()) {
                this->assign(0, begin->GetLastLine(), AtBegin, begin->GetTotalLength());
                this->last_line = begin->GetLastLine();
                this->total_length = begin->GetTotalLength();
            }

            if (content.has_value()) {
                if (begin && !begin->Empty()) {
                    this->last_line++;
                }
                Offset start_offset = 0;
                this->total_length += content.value().size();
                newline.Iterate(content.value(), [&](std::size_t i, std::size_t width) {
                    this->assign(this->last_line, this->last_line, start_offset, i - start_offset);
                    this->last_line++;
                    start_offset = i + width;
                });
                this->assign(this->last_line, this->last_line, start_offset, content.value().size() - start_offset);
            }

            if (end && !end->Empty()) {
                if (content.has_value() || (begin && !begin->Empty())) {
                    this->last_line++;
                }
                this->assign(this->last_line, this->last_line + end->GetLastLine(), AtEnd, end->GetTotalLength());
                this->last_line += end->GetLastLine();
                this->total_length += end->GetTotalLength();
            }
        }

        Line GetLastLine() const {
            return this->last_line;
        }

        Length GetTotalLength() const {
            return this->total_length;
        }

        Offset GetOffset(Line l) const {
            if (this->lines.Has(l)) {
                return this->lines.At(l).first;
            } else {
                return NotFound;
            }
        }

        Length GetLength(Line l) const {
            if (this->lines.Has(l)) {
                return this->lines.At(l).second;
            } else {
                return 0;
            }
        }

        static constexpr Offset MaxOffset = std::numeric_limits<Offset>::max() - 3;
        static constexpr Offset AtBegin = std::numeric_limits<Offset>::max() - 2;
        static constexpr Offset AtEnd = std::numeric_limits<Offset>::max() - 1;
        static constexpr Offset NotFound = std::numeric_limits<Offset>::max();

     private:
        void assign(Line begin, Line end, Offset start, Length length) {
            this->lines.Insert(begin, end + 1, std::make_pair(start, length));
        }

        RangeMap<Line, std::pair<Offset, Length>> lines;
        Line last_line;
        Length total_length;
    };
}

#endif