#ifndef SLOKED_SCREEN_TERMINAL_POSIX_POSIXTERMINAL_H_
#define SLOKED_SCREEN_TERMINAL_POSIX_POSIXTERMINAL_H_

#include "sloked/Base.h"
#include "sloked/screen/terminal/Terminal.h"
#include <string>
#include <memory>
#include <sstream>

namespace sloked {
    
    class PosixTerminal : public SlokedTerminal {
     public:
        class Termcap;

        PosixTerminal(FILE * = stdout, FILE * = stdin);
        virtual ~PosixTerminal();
    
        void SetPosition(Line, Column) override;
        void MoveUp(Line) override;
        void MoveDown(Line) override;
        void MoveBackward(Column) override;
        void MoveForward(Column) override;

        void ShowCursor(bool) override;
        void ClearScreen() override;
        void ClearChars(Column) override;
        Column GetWidth() override;
        Line GetHeight() override;

        void Write(const std::string &) override;
        std::vector<SlokedKeyboardInput> GetInput() override;

        void SetGraphicsMode(SlokedTextGraphics) override;
        void SetGraphicsMode(SlokedBackgroundGraphics) override;
        void SetGraphicsMode(SlokedForegroundGraphics) override;

        void Update() override;
        void Flush(bool) override;

     private:
        struct State;
        FILE *GetOutputFile();

        std::unique_ptr<State> state;
        std::unique_ptr<Termcap> termcap;
        bool disable_flush;
        std::stringstream buffer;
        Column width;
        Line height;
    };
}

#endif