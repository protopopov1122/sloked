#ifndef SLOKED_SCREEN_WIDGETS_TEXTPANE_H_
#define SLOKED_SCREEN_WIDGETS_TEXTPANE_H_

#include "sloked/Base.h"
#include "sloked/screen/Component.h"
#include "sloked/screen/Graphics.h"

namespace sloked {

    class SlokedTextPane {
     public:
        using Line = TextGraphics::Line;
        using Column = TextGraphics::Column;

        virtual ~SlokedTextPane() = default;
    
        virtual void SetPosition(Line, Column) = 0;
        virtual void MoveUp(Line) = 0;
        virtual void MoveDown(Line) = 0;
        virtual void MoveBackward(Column) = 0;
        virtual void MoveForward(Column) = 0;

        virtual void ShowCursor(bool) = 0;
        virtual void ClearScreen() = 0;
        virtual void ClearChars(Column) = 0;
        virtual Column GetWidth() = 0;
        virtual Line GetHeight() = 0;

        virtual void Write(const std::string &) = 0;

        virtual void SetGraphicsMode(SlokedTextGraphics) = 0;
        virtual void SetGraphicsMode(SlokedBackgroundGraphics) = 0;
        virtual void SetGraphicsMode(SlokedForegroundGraphics) = 0;
    };
}

#endif