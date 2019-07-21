#ifndef SLOKED_TEXT_TEXTBLOCKHANDLE_H_
#define SLOKED_TEXT_TEXTBLOCKHANDLE_H_

#include "sloked/text/TextChunk.h"
#include <variant>

namespace sloked {

    class TextBlockHandle : public TextBlockImpl<TextBlockHandle> {
     public:
        TextBlockHandle(std::string_view, std::map<std::size_t, std::pair<std::size_t, std::size_t>>, const TextBlockFactory &factory = TextChunk::Factory);

        std::size_t GetLastLine() const override;
        std::size_t GetTotalLength() const override;
        const std::string_view GetLine(std::size_t) const override;
        bool Empty() const override;

        void SetLine(std::size_t, const std::string &) override;
        void EraseLine(std::size_t) override;
        void InsertLine(std::size_t, const std::string &) override;
        void Optimize() override;

        friend std::ostream &operator<<(std::ostream &, const TextBlockHandle &);
    
     private:
        void open_block() const;

        struct view {
            std::string_view content;
            std::map<std::size_t, std::pair<std::size_t, std::size_t>> lines;
        };

        mutable std::variant<view, std::unique_ptr<TextBlock>> content;
        const TextBlockFactory &factory;
    };
}

#endif