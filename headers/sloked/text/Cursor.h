#ifndef SLOKED_TEXT_CURSOR_H_
#define SLOKED_TEXT_CURSOR_H_

#include "sloked/Base.h"
#include "sloked/text/TextBlock.h"
#include "sloked/core/Encoding.h"

namespace sloked {

    class SlokedCursor {
     public:
        SlokedCursor(TextBlock &, const Encoding &);
    
        unsigned int GetLine() const;
        unsigned int GetColumn() const;

        void MoveUp(unsigned int);
        void MoveDown(unsigned int);
        void MoveForward(unsigned int);
        void MoveBackward(unsigned int);

        void Insert(const std::string &);
        void NewLine();
        void Remove();

     private:
        void UpdateCursor();

        TextBlock &text;
        const Encoding &encoding;
        unsigned int line;
        unsigned int column;
        unsigned int width;
    };
}

#endif