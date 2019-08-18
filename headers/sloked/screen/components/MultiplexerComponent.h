#ifndef SLOKED_SCREEN_COMPONENTS_MULTIPLEXERCOMPONENT_H_
#define SLOKED_SCREEN_COMPONENTS_MULTIPLEXERCOMPONENT_H_

#include "sloked/core/Indexed.h"
#include "sloked/screen/Component.h"
#include "sloked/screen/components/ComponentHandle.h"
#include <optional>

namespace sloked {

    class SlokedMultiplexerComponent : public SlokedScreenComponent {
     public:
        class Window : public SlokedComponentWindow {
         public:
            virtual void Move(const TextPosition &) = 0;
            virtual void Resize(const TextPosition &) = 0;
        };

        virtual std::shared_ptr<Window> GetFocus() const = 0;
        virtual std::shared_ptr<Window> GetWindow(Window::Id) const = 0;
        virtual std::size_t GetWindowCount() const = 0;

        virtual std::shared_ptr<Window> NewWindow(const TextPosition &, const TextPosition &) = 0;
    };
}

#endif