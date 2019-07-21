#ifndef SLOKED_TEXT_TEXTCHUNK_H_
#define SLOKED_TEXT_TEXTCHUNK_H_

#include "sloked/text/TextBlock.h"
#include "sloked/core/AVL.h"
#include <map>
#include <limits>
#include <optional>
#include <iosfwd>

namespace sloked {

    class TextChunk;

    class TextChunkFactory : public TextBlockFactory {
     public:
        std::unique_ptr<TextBlock> make(const std::string_view) const override;
    };

    class TextChunk : public TextBlockImpl<TextChunk>, public AVLNode<TextChunk> {
     public:
        TextChunk(const std::string_view, std::size_t = 0);
        TextChunk(std::unique_ptr<TextChunk>, std::optional<std::string>, std::unique_ptr<TextChunk>, std::size_t = 0);

        std::size_t GetLastLine() const override;
        std::size_t GetTotalLength() const override;
        const std::string_view GetLine(std::size_t) const override;
        bool Empty() const override;
        
        void SetLine(std::size_t, const std::string &) override;
        void EraseLine(std::size_t) override;
        void InsertLine(std::size_t, const std::string &) override;
        void Optimize() override;

        void Squash();
        void Compact();
        void Balance();
        std::size_t GetHeight() const;

        friend std::ostream &operator<<(std::ostream &, const TextChunk &);
        
        static TextChunkFactory Factory;

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

        bool has_content;
        std::size_t height;
        std::size_t first_line;
        std::optional<std::string> content;
        std::unique_ptr<DefaultMap> chunk_map;
    };

    template <typename Line, typename Offset, typename Length = Offset>
    class TextChunk::Map {
     public:
        Map(const std::optional<std::string> &content, TextChunk *begin, TextChunk *end)
            : last_line(0), total_length(0) {
            if (begin && !begin->Empty()) {
                this->assign(0, begin->GetLastLine(), AtBegin, begin->GetTotalLength());
                this->last_line = begin->GetLastLine();
                this->total_length = begin->GetTotalLength();
            }

            if (content.has_value()) {
                if (begin && !begin->Empty()) {
                    this->last_line++;
                }
                Offset i = 0, start_offset = 0;
                for (; i < content.value().size(); i++) {
                    this->total_length++;
                    if (content.value()[i] == '\n') {
                        this->assign(this->last_line, this->last_line, start_offset, i - start_offset);
                        this->last_line++;
                        start_offset = i + 1;
                    }
                }
                this->assign(this->last_line, this->last_line, start_offset, i - start_offset);
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
            if (this->lines.count(l)) {
                return this->lines.at(l).first;
            } else {
                return NotFound;
            }
        }

        Length GetLength(Line l) const {
            if (this->lines.count(l)) {
                return this->lines.at(l).second;
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
            for (Line i = begin; i <= end; i++) {
                this->lines[i] = std::make_pair(start, length);
            }
        }

        std::map<Line, std::pair<Offset, Length>> lines;
        Line last_line;
        Length total_length;
    };
}

#endif