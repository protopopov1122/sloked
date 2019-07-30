#ifndef SLOKED_TEXT_CURSOR_CURSOR_H_
#define SLOKED_TEXT_CURSOR_CURSOR_H_

#include "sloked/Base.h"
#include "sloked/core/Position.h"
#include <string>
#include <vector>

namespace sloked {

    class SlokedCursor {
     public:
        using Line = TextPosition::Line;
        using Column = TextPosition::Column;

        virtual ~SlokedCursor() = default;
    
        virtual Line GetLine() const = 0;
        virtual Column GetColumn() const = 0;

        virtual void SetPosition(Line, Column) = 0;
        virtual void MoveUp(Line) = 0;
        virtual void MoveDown(Line) = 0;
        virtual void MoveForward(Column) = 0;
        virtual void MoveBackward(Column) = 0;

        virtual void Insert(std::string_view) = 0;
        virtual void NewLine(std::string_view) = 0;
        virtual void DeleteBackward() = 0;
        virtual void DeleteForward() = 0;
        virtual void ClearRegion(const TextPosition &);
        virtual void ClearRegion(const TextPosition &, const TextPosition &) = 0;
    };
}

#endif