#ifndef SLOKED_SCREEN_COMPONENT_H_
#define SLOKED_SCREEN_COMPONENT_H_

#include "sloked/core/Position.h"
#include "sloked/screen/Keyboard.h"
#include <functional>

namespace sloked {

    class SlokedScreenComponent {
     public:
        using InputHandler = std::function<bool(const SlokedKeyboardInput &)>;

        SlokedScreenComponent();
        virtual ~SlokedScreenComponent() = default;

        void ProcessInput(const SlokedKeyboardInput &);
        void SetInputHandler(InputHandler);
        void ResetInputHandler();
        
        virtual void Render() = 0;
        virtual void UpdateDimensions() = 0;

     protected:
        virtual void ProcessComponentInput(const SlokedKeyboardInput &) = 0;

     private:
        InputHandler inputHandler;
    };
}

#endif