#include "sloked/screen/Component.h"

namespace sloked {

    SlokedScreenComponent::SlokedScreenComponent()
        : inputHandler([](const auto &) { return false; }) {}
    
    void SlokedScreenComponent::SetInputHandler(InputHandler handler) {
        this->inputHandler = handler;
    }
}