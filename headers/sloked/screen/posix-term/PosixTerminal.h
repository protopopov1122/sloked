#ifndef SLOKED_SCREEN_POSIXTERM_ANSICONSOLE_H_
#define SLOKED_SCREEN_POSIXTERM_ANSICONSOLE_H_

#include "sloked/Base.h"
#include "sloked/screen/Terminal.h"
#include <string>
#include <memory>

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

        void ClearScreen() override;
        void ClearLine() override;
        Column GetWidth() override;
        Line GetHeight() override;

        void Write(const std::string &) override;
        std::vector<SlokedKeyboardInput> GetInput() override;

        void SetGraphicsMode(SlokedTerminalText) override;
        void SetGraphicsMode(SlokedTerminalBackground) override;
        void SetGraphicsMode(SlokedTerminalForeground) override;

     private:
        struct State;
        FILE *GetOutputFile();

        std::unique_ptr<State> state;
        std::unique_ptr<Termcap> termcap;
    };
}

#endif