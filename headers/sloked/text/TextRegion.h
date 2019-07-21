#ifndef SLOKED_TEXT_TEXTREGION_H_
#define SLOKED_TEXT_TEXTREGION_H_

#include "sloked/text/TextBlock.h"
#include "sloked/core/AVL.h"

namespace sloked {

    class TextRegion : public TextBlockImpl<TextRegion>, public AVLNode<TextRegion> {
     public:
        TextRegion(std::unique_ptr<TextBlock>);

        std::size_t GetLastLine() const override;
        std::size_t GetTotalLength() const override;
        const std::string_view GetLine(std::size_t) const override;
        bool Empty() const override;

        void SetLine(std::size_t, const std::string &) override;
        void EraseLine(std::size_t) override;
        void InsertLine(std::size_t, const std::string &) override;
        void Optimize() override;

        friend std::ostream &operator<<(std::ostream &, const TextRegion &);

        void AppendRegion(std::unique_ptr<TextRegion>);
        std::size_t GetHeight() const;
        
     protected:
        void AvlSwapContent(TextRegion &) override;
        void AvlUpdate() override;

     private:
        void UpdateStats() const;

        std::unique_ptr<TextBlock> content;

        mutable std::size_t height;
        mutable std::size_t last_line;
    };
}

#endif