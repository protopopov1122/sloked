#ifndef SLOKED_SCREEN_WIDGETS_SPLITTERCOMPONENT_H_
#define SLOKED_SCREEN_WIDGETS_SPLITTERCOMPONENT_H_

#include "sloked/screen/Component.h"
#include "sloked/screen/widgets/ComponentHandle.h"

namespace sloked {

    class SlokedSplitterComponent : public SlokedScreenComponent {
     public:
        virtual void SetFocus(std::size_t) = 0;
        virtual std::size_t GetFocus() const = 0;
        virtual SlokedComponentHandle *GetWindow(std::size_t) const = 0;
        virtual std::size_t GetWindowCount() const = 0;
        virtual SlokedComponentHandle &NewWindow(const Splitter::Constraints &) = 0;
    };
}

#endif