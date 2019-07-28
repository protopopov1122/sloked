#ifndef SLOKED_SCREEN_COMPONENT_H_
#define SLOKED_SCREEN_COMPONENT_H_

#include "sloked/Base.h"
#include "sloked/screen/Keyboard.h"
#include <functional>

namespace sloked {

    class SlokedScreenComponent {
     public:
        using InputHandler = std::function<bool(const SlokedKeyboardInput &)>;

        SlokedScreenComponent();
        virtual ~SlokedScreenComponent() = default;

        virtual void ProcessInput(const SlokedKeyboardInput &) = 0;
        virtual void Render() = 0;

        void SetInputHandler(InputHandler);
    
     protected:
        InputHandler inputHandler;
    };
}

#endif