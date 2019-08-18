#ifndef SLOKED_SCREEN_COMPONENTS_COMPONENTHANDLE_H_
#define SLOKED_SCREEN_COMPONENTS_COMPONENTHANDLE_H_

#include "sloked/screen/Component.h"
#include "sloked/screen/Splitter.h"
#include "sloked/screen/widgets/TextPaneWidget.h"
#include <memory>

namespace sloked {

    class SlokedSplitterComponent;
    class SlokedTabberComponent;
    class SlokedMultiplexerComponent;

    class SlokedComponentHandle : public SlokedScreenComponent {
     public:
        virtual SlokedScreenComponent &NewTextPane(std::unique_ptr<SlokedTextPaneWidget>) = 0;
        virtual SlokedSplitterComponent &NewSplitter(Splitter::Direction) = 0;
        virtual SlokedTabberComponent &NewTabber() = 0;
        virtual SlokedMultiplexerComponent &NewMultiplexer() = 0;
    };

    class SlokedComponentWindow {
     public:
        using Id = std::size_t;
        virtual ~SlokedComponentWindow() = default;
        virtual bool IsOpen() const = 0;
        virtual bool HasFocus() const = 0;
        virtual SlokedComponentHandle &GetComponent() const = 0;
        virtual Id GetId() const = 0;

        virtual void SetFocus() = 0;
        virtual void Close() = 0;
    };
}

#endif