#ifndef SLOKED_TEXT_CURSOR_PLAINCURSOR_H_
#define SLOKED_TEXT_CURSOR_PLAINCURSOR_H_

#include "sloked/Base.h"
#include "sloked/text/cursor/Cursor.h"
#include "sloked/text/cursor/Reader.h"
#include "sloked/core/Encoding.h"
#include "sloked/text/TextBlock.h"

namespace sloked {

    class PlainCursor : public SlokedCursor, public SlokedTextReader {
     public:
        PlainCursor(TextBlock &, const Encoding &);
    
        Line GetLine() const override;
        Column GetColumn() const override;

        void SetPosition(Line, Column) override;
        void MoveUp(Line) override;
        void MoveDown(Line) override;
        void MoveForward(Column) override;
        void MoveBackward(Column) override;

        void Insert(std::string_view) override;
        void NewLine(std::string_view) override;
        void DeleteBackward() override;
        void DeleteForward() override;
        using SlokedCursor::ClearRegion;
        void ClearRegion(const TextPosition &, const TextPosition &) override;

        std::vector<std::string> Read(const TextPosition &, const TextPosition &) const override;
        Column LineLength(Line) const override;
        Line LineCount() const override;

     private:
        void UpdateCursor();

        TextBlock &text;
        const Encoding &encoding;
        Line line;
        Column column;
        Column width;
    };
}

#endif