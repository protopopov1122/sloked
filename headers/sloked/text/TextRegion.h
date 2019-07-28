#ifndef SLOKED_TEXT_TEXTREGION_H_
#define SLOKED_TEXT_TEXTREGION_H_

#include "sloked/core/NewLine.h"
#include "sloked/core/AVL.h"
#include "sloked/text/TextBlock.h"

namespace sloked {

    class TextRegion : public TextBlockImpl<TextRegion>, public AVLNode<TextRegion> {
     public:
         TextRegion(const NewLine &, std::unique_ptr<TextBlock>);

         std::size_t GetLastLine() const override;
         std::size_t GetTotalLength() const override;
         std::string_view GetLine(std::size_t) const override;
         bool Empty() const override;
         void Visit(std::size_t, std::size_t, Visitor) const override;

         void SetLine(std::size_t, std::string_view) override;
         void EraseLine(std::size_t) override;
         void InsertLine(std::size_t, std::string_view) override;
         void Optimize() override;

         friend std::ostream &operator<<(std::ostream &, const TextRegion &);

         void AppendRegion(std::unique_ptr<TextRegion>);
         std::size_t GetHeight() const;
        
     protected:
         void AvlSwapContent(TextRegion &) override;
         void AvlUpdate() override;

     private:
         void UpdateStats() const;

         const NewLine &newline;
         std::unique_ptr<TextBlock> content;

         mutable std::size_t height;
         mutable std::size_t last_line;
    };
}

#endif