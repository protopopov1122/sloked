#ifndef SLOKED_SCREEN_COMPONENTS_TABBERCOMPONENT_H_
#define SLOKED_SCREEN_COMPONENTS_TABBERCOMPONENT_H_

#include "sloked/core/Indexed.h"
#include "sloked/screen/Component.h"
#include "sloked/screen/components/ComponentHandle.h"
#include <optional>

namespace sloked {

    class SlokedTabberComponent : public SlokedScreenComponent {
     public:
        using TabId = std::size_t;

        virtual TabId GetTabCount() const = 0;
        virtual std::optional<TabId> GetCurrentTab() const = 0;
        virtual SlokedComponentHandle &GetTab(TabId) const = 0;

        virtual bool SelectTab(TabId) = 0;
        virtual SlokedIndexed<SlokedComponentHandle &, TabId> NewTab() = 0;
        virtual SlokedIndexed<SlokedComponentHandle &, TabId> NewTab(TabId) = 0;
        virtual bool CloseTab(TabId) = 0;
    };
}

#endif