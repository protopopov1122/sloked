#ifndef SLOKED_SCREEN_COMPONENTS_TEXTPANE_H_
#define SLOKED_SCREEN_COMPONENTS_TEXTPANE_H_

#include "sloked/screen/Component.h"
#include "sloked/screen/Character.h"

namespace sloked {

    class SlokedTextPaneComponent : public SlokedScreenComponent {
     public:
        virtual const SlokedFontProperties &GetFontProperties() const = 0;

     protected:
        using SlokedScreenComponent::SlokedScreenComponent;
    };
}

#endif