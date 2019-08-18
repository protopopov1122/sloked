#ifndef SLOKED_SCREEN_TERMINAL_COMPONENTS_MULTIPLEXERCOMPONENT_H_
#define SLOKED_SCREEN_TERMINAL_COMPONENTS_MULTIPLEXERCOMPONENT_H_

#include "sloked/screen/components/MultiplexerComponent.h"
#include "sloked/screen/terminal/multiplexer/TerminalWindow.h"
#include "sloked/screen/terminal/components/ComponentHandle.h"
#include <map>
#include <memory>
#include <list>

namespace sloked {

   class TerminalMultiplexerComponent : public SlokedMultiplexerComponent {
    public:
      TerminalMultiplexerComponent(SlokedTerminal &, const Encoding &, const SlokedCharWidth &);

      std::shared_ptr<Window> GetFocus() const override;
      std::shared_ptr<Window> GetWindow(Window::Id) const override;
      std::size_t GetWindowCount() const override;

      std::shared_ptr<Window> NewWindow(const TextPosition &, const TextPosition &) override;

      void Render() override;
      void Update() override;

    protected:
      void ProcessComponentInput(const SlokedKeyboardInput &) override;
        
    private:
      class TerminalMultiplexerWindow : public Window {
       public:
         TerminalMultiplexerWindow(Id, std::unique_ptr<TerminalComponentHandle>, std::unique_ptr<TerminalWindow>, TerminalMultiplexerComponent &);
         virtual ~TerminalMultiplexerWindow();

         bool IsOpen() const override;
         bool HasFocus() const override;
         SlokedComponentHandle &GetComponent() const override;
         Id GetId() const override;

         void SetFocus() override;
         void Move(const TextPosition &) override;
         void Resize(const TextPosition &) override;
         void Close() override;

         void Render();
         void Update();
         void ProcessInput(const SlokedKeyboardInput &);

       private:
         Id id;
         std::unique_ptr<TerminalComponentHandle> component;
         std::unique_ptr<TerminalWindow> window;
         TerminalMultiplexerComponent &root;
      };

      friend class TerminalMultiplexerWindow;

      SlokedTerminal &terminal;
      const Encoding &encoding;
      const SlokedCharWidth &charWidth;
      std::map<Window::Id, std::shared_ptr<TerminalMultiplexerWindow>> windows;
      std::list<Window::Id> focus;
      Window::Id nextId;
   };
}

#endif