#ifndef SLOKED_SCREEN_WIDGETS_TABBERCOMPONENT_H_
#define SLOKED_SCREEN_WIDGETS_TABBERCOMPONENT_H_

#include "sloked/screen/Component.h"
#include "sloked/screen/widgets/ComponentHandle.h"

namespace sloked {

    class SlokedTabberComponent : public SlokedScreenComponent {
     public:
        virtual SlokedComponentHandle &NewTab() = 0;
        virtual SlokedComponentHandle &NewTab(std::size_t) = 0;
        virtual void SelectTab(std::size_t) = 0;
        virtual std::size_t GetCurrentTab() const = 0;
        virtual SlokedComponentHandle *GetTab(std::size_t) const = 0;
    };
}

#endif