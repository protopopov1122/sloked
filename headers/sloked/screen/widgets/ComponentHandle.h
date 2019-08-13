#ifndef SLOKED_SCREEN_WIDGETS_COMPONENTHANDLE_H_
#define SLOKED_SCREEN_WIDGETS_COMPONENTHANDLE_H_

#include "sloked/screen/Component.h"
#include "sloked/screen/Splitter.h"
#include "sloked/screen/widgets/TextPaneComponent.h"

namespace sloked {

    class SlokedSplitterComponent;
    class SlokedTabberComponent;

    class SlokedComponentHandle : public SlokedScreenComponent {
     public:
        virtual SlokedTextPaneComponent &NewTextPane(std::unique_ptr<SlokedTextPaneWidget>) = 0;
        virtual SlokedSplitterComponent &NewSplitter(Splitter::Direction) = 0;
        virtual SlokedTabberComponent &NewTabber() = 0;
    };
}

#endif