#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_SPLITTERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_SPLITTERCOMPONENT_H_

#include "sloked/core/CharWidth.h"
#include "sloked/core/Encoding.h"
#include "sloked/screen/components/SplitterComponent.h"
#include "sloked/screen/terminal/Terminal.h"
#include "sloked/screen/terminal/multiplexer/TerminalSplitter.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include <vector>
#include <memory>

namespace sloked {

    class TerminalSplitterComponent : public SlokedSplitterComponent {
    public:
      TerminalSplitterComponent(SlokedTerminal &, Splitter::Direction, const Encoding &, const SlokedCharWidth &);

      std::shared_ptr<Window> GetFocus() const override;
      std::shared_ptr<Window> GetWindow(Window::Id) const override;
      std::size_t GetWindowCount() const override;

      std::shared_ptr<Window> NewWindow(const Splitter::Constraints &) override;
      std::shared_ptr<Window> NewWindow(Window::Id, const Splitter::Constraints &) override;

      void Render() override;
      void UpdateDimensions() override;

    protected:
      void ProcessComponentInput(const SlokedKeyboardInput &) override;
        
    private:
      class TerminalSplitterWindow : public Window {
       public:
         TerminalSplitterWindow(Window::Id, std::unique_ptr<TerminalComponentHandle>, TerminalSplitterComponent &);
         bool IsOpen() const override;
         bool HasFocus() const override;
         SlokedComponentHandle &GetComponent() const override;
         Id GetId() const override;

         void SetFocus() override;
         void UpdateConstraints(const Splitter::Constraints &) override;
         void Move(Id) override;
         void Close() override;

         void Update();
         void Render();
         void ProcessInput(const SlokedKeyboardInput &);

       private:
         Window::Id id;
         std::unique_ptr<TerminalComponentHandle> component;
         TerminalSplitterComponent &root;
      };

      friend class TerminalSplitterWindow;

      TerminalSplitter splitter;
      const Encoding &encoding;
      const SlokedCharWidth &charWidth;
      std::vector<std::shared_ptr<TerminalSplitterWindow>> components;
      Window::Id focus;
    };
}

#endif