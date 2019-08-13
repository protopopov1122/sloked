#ifndef SLOKED_SCREEN_WIDGETS_TEXTPANECOMPONENT_H_
#define SLOKED_SCREEN_WIDGETS_TEXTPANECOMPONENT_H_

#include "sloked/screen/Component.h"
#include "sloked/screen/widgets/TextPane.h"

namespace sloked {

    class SlokedTextPaneWidget {
     public:
        virtual ~SlokedTextPaneWidget() = default;
        virtual bool ProcessInput(const SlokedKeyboardInput &) = 0;
        virtual void Render(SlokedTextPane &) = 0;
    };

    class SlokedTextPaneComponent : public SlokedScreenComponent {};
}

#endif