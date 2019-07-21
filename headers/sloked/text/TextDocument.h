#ifndef SLOKED_TEXT_TEXTDOCUMENT_H_
#define SLOKED_TEXT_TEXTDOCUMENT_H_

#include "sloked/text/TextBlock.h"

namespace sloked {

    class TextDocument : public TextBlockImpl<TextDocument> {
     public:
        TextDocument(std::unique_ptr<TextBlock>);

        std::size_t GetLastLine() const override;
        std::size_t GetTotalLength() const override;
        const std::string_view GetLine(std::size_t) const override;
        bool Empty() const override;
        
        void SetLine(std::size_t, const std::string &) override;
        void EraseLine(std::size_t) override;
        void InsertLine(std::size_t, const std::string &) override;
        void Optimize() override;

        friend std::ostream &operator<<(std::ostream &os, const TextDocument &);
    
     private:
        std::unique_ptr<TextBlock> content;
    };
}

#endif