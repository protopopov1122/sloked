#ifndef SLOKED_SCREEN_COMPONENTS_SPLITTERCOMPONENT_H_
#define SLOKED_SCREEN_COMPONENTS_SPLITTERCOMPONENT_H_

#include "sloked/core/Indexed.h"
#include "sloked/screen/Component.h"
#include "sloked/screen/components/ComponentHandle.h"
#include <optional>

namespace sloked {

    class SlokedSplitterComponent : public SlokedScreenComponent {
     public:
        class Window : public SlokedComponentWindow {
         public:
            virtual void UpdateConstraints(const Splitter::Constraints &) = 0;
            virtual void Move(Id) = 0;
        };

        virtual std::shared_ptr<Window> GetFocus() const = 0;
        virtual std::shared_ptr<Window> GetWindow(Window::Id) const = 0;
        virtual std::size_t GetWindowCount() const = 0;

        virtual std::shared_ptr<Window> NewWindow(const Splitter::Constraints &) = 0;
        virtual std::shared_ptr<Window> NewWindow(Window::Id, const Splitter::Constraints &) = 0;
    };
}

#endif