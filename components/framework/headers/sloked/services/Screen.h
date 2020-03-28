/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_SERVICES_SCREEN_H_
#define SLOKED_SERVICES_SCREEN_H_

#include "sloked/core/Encoding.h"
#include "sloked/core/Monitor.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/kgr/Server.h"
#include "sloked/kgr/Service.h"
#include "sloked/screen/Component.h"
#include "sloked/screen/Splitter.h"
#include "sloked/screen/components/ComponentHandle.h"
#include "sloked/services/Service.h"

namespace sloked {

    class SlokedScreenService : public KgrService {
     public:
        SlokedScreenService(SlokedMonitor<SlokedScreenComponent &> &,
                            const Encoding &, KgrServer::Connector,
                            KgrServer::Connector, KgrServer::Connector,
                            KgrContextManager<KgrLocalContext> &);
        void Attach(std::unique_ptr<KgrPipe>) override;

     private:
        SlokedMonitor<SlokedScreenComponent &> &root;
        const Encoding &encoding;
        KgrServer::Connector cursorService;
        KgrServer::Connector renderService;
        KgrServer::Connector notifyService;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedScreenClient {
     public:
        class HandleClient {
         public:
            HandleClient(SlokedServiceClient &, std::function<void()>);
            bool NewMultiplexer(const std::string &) const;
            bool NewSplitter(const std::string &, Splitter::Direction) const;
            bool NewTabber(const std::string &) const;
            bool NewTextEditor(const std::string &,
                               SlokedEditorDocumentSet::DocumentId,
                               const std::string & = "") const;

         private:
            SlokedServiceClient &client;
            std::function<void()> preventDeadlock;
        };

        class MultiplexerClient {
         public:
            MultiplexerClient(SlokedServiceClient &, std::function<void()>);
            std::optional<std::string> NewWindow(const std::string &,
                                                 const TextPosition &,
                                                 const TextPosition &) const;
            std::size_t GetWindowCount(const std::string &) const;
            std::optional<std::string> GetFocus(const std::string &) const;
            std::optional<bool> WindowHasFocus(const std::string &) const;
            bool SetFocus(const std::string &) const;
            bool Close(const std::string &) const;
            bool MoveWindow(const std::string &, const TextPosition &) const;
            bool ResizeWindow(const std::string &, const TextPosition &) const;

         private:
            SlokedServiceClient &client;
            std::function<void()> preventDeadlock;
        };

        class SplitterClient {
         public:
            SplitterClient(SlokedServiceClient &, std::function<void()>);
            std::optional<std::string> NewWindow(
                const std::string &, const Splitter::Constraints &) const;
            std::optional<std::string> NewWindow(
                const std::string &, SlokedComponentWindow::Id,
                const Splitter::Constraints &) const;
            std::size_t GetWindowCount(const std::string &) const;
            std::optional<std::string> GetFocus(const std::string &) const;
            std::optional<bool> WindowHasFocus(const std::string &) const;
            bool SetFocus(const std::string &) const;
            bool Close(const std::string &) const;
            std::optional<std::string> MoveWindow(
                const std::string &, SlokedComponentWindow::Id) const;
            bool UpdateWindowConstraints(const std::string &,
                                         const Splitter::Constraints &) const;

         private:
            SlokedServiceClient &client;
            std::function<void()> preventDeadlock;
        };

        class TabberClient {
         public:
            TabberClient(SlokedServiceClient &, std::function<void()>);
            std::optional<std::string> NewWindow(const std::string &) const;
            std::optional<std::string> NewWindow(
                const std::string &, SlokedComponentWindow::Id) const;
            std::size_t GetWindowCount(const std::string &) const;
            std::optional<std::string> GetFocus(const std::string &) const;
            std::optional<bool> WindowHasFocus(const std::string &) const;
            bool SetFocus(const std::string &) const;
            bool Close(const std::string &) const;
            std::optional<std::string> MoveWindow(
                const std::string &, SlokedComponentWindow::Id) const;

         private:
            SlokedServiceClient &client;
            std::function<void()> preventDeadlock;
        };

        SlokedScreenClient(std::unique_ptr<KgrPipe>,
                           std::function<bool()> = nullptr);

     private:
        void PreventDeadlock();
        SlokedServiceClient client;
        std::function<bool()> holdsLock;

     public:
        const HandleClient Handle;
        const MultiplexerClient Multiplexer;
        const SplitterClient Splitter;
        const TabberClient Tabber;
    };
}  // namespace sloked

#endif