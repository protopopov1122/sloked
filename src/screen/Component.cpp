#include "sloked/screen/Component.h"

namespace sloked {

    SlokedScreenComponent::SlokedScreenComponent() = default;
    
    void SlokedScreenComponent::ProcessInput(const SlokedKeyboardInput &input) {
        bool res = false;
        if (this->inputHandler) {
            res = this->inputHandler(input);
        }
        if (!res) {
            this->ProcessComponentInput(input);
        }
    }

    void SlokedScreenComponent::SetInputHandler(InputHandler handler) {
        this->inputHandler = handler;
    }

    void SlokedScreenComponent::ResetInputHandler() {
        this->inputHandler = 0;
    }
}