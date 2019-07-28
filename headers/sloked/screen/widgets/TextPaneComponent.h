#ifndef SLOKED_SCREEN_WIDGETS_TEXTPANECOMPONENT_H_
#define SLOKED_SCREEN_WIDGETS_TEXTPANECOMPONENT_H_

#include "sloked/screen/Component.h"
#include "sloked/screen/widgets/TextPane.h"

namespace sloked {

    class SlokedTextPaneComponent : public SlokedScreenComponent {
     public:
        using Renderer = std::function<void(SlokedTextPane &)>;
        virtual void SetRenderer(Renderer) = 0;
    };
}

#endif