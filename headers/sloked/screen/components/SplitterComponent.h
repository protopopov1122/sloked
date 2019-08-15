#ifndef SLOKED_SCREEN_COMPONENTS_SPLITTERCOMPONENT_H_
#define SLOKED_SCREEN_COMPONENTS_SPLITTERCOMPONENT_H_

#include "sloked/core/Indexed.h"
#include "sloked/screen/Component.h"
#include "sloked/screen/components/ComponentHandle.h"
#include <optional>

namespace sloked {

    class SlokedSplitterComponent : public SlokedScreenComponent {
     public:
        using WinId = std::size_t;

        virtual std::optional<WinId> GetFocus() const = 0;
        virtual SlokedComponentHandle &GetWindow(WinId) const = 0;
        virtual WinId GetWindowCount() const = 0;
        virtual const Splitter::Constraints &GetConstraints(WinId) const = 0;

        virtual bool SetFocus(WinId) = 0;
        virtual SlokedIndexed<SlokedComponentHandle &, WinId> NewWindow(const Splitter::Constraints &) = 0;
        virtual SlokedIndexed<SlokedComponentHandle &, WinId> NewWindow(WinId, const Splitter::Constraints &) = 0;
        virtual bool UpdateConstraints(WinId, const Splitter::Constraints &) = 0;
        virtual bool CloseWindow(WinId) = 0;
    };
}

#endif