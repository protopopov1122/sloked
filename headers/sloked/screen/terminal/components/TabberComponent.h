#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_TABBERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_TABBERCOMPONENT_H_

#include "sloked/core/CharWidth.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/components/TabberComponent.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalTabber.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include <vector>
#include <memory>

namespace sloked {

    class TerminalTabberComponent : public SlokedTabberComponent {
     public:
        TerminalTabberComponent(SlokedTerminal &, const Encoding &, const SlokedCharWidth &);

        std::size_t GetWindowCount() const override;
        std::shared_ptr<Window> GetFocus() const override;
        std::shared_ptr<Window> GetWindow(Window::Id) const override;

        std::shared_ptr<Window> NewWindow() override;
        std::shared_ptr<Window> NewWindow(Window::Id) override;

        void Render() override;
        void Update() override;

     protected:
        void ProcessComponentInput(const SlokedKeyboardInput &) override;
        
     private:
        class TerminalTabberWindow : public Window {
         public:
            TerminalTabberWindow(Id, std::unique_ptr<TerminalComponentHandle>, TerminalTabberComponent &);
            bool IsOpen() const override;
            bool HasFocus() const override;
            SlokedComponentHandle &GetComponent() const override;
            Id GetId() const override;

            void SetFocus() override;
            void Move(Id) override;
            void Close() override;

            void Render();
            void Update();
            void ProcessInput(const SlokedKeyboardInput &);

         private:
            Id id;
            std::unique_ptr<TerminalComponentHandle> component;
            TerminalTabberComponent &root;
        };

        friend class TerminalTabberWindow;

        TerminalTabber tabber;
        const Encoding &encoding;
        const SlokedCharWidth &charWidth;
        std::vector<std::shared_ptr<TerminalTabberWindow>> components;
    };
}

#endif