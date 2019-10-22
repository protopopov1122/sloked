/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_SERVICES_SCREEN_H_
#define SLOKED_SERVICES_SCREEN_H_

#include "sloked/services/Service.h"
#include "sloked/kgr/Service.h"
#include "sloked/kgr/Server.h"
#include "sloked/core/Synchronized.h"
#include "sloked/screen/Component.h"
#include "sloked/screen/Splitter.h"
#include "sloked/core/Encoding.h"
#include "sloked/editor/doc-mgr/DocumentSet.h"
#include "sloked/screen/components/ComponentHandle.h"

namespace sloked {

    class SlokedScreenService : public KgrService {
     public:
        SlokedScreenService(SlokedSynchronized<SlokedScreenComponent &> &, const Encoding &, KgrServer::Connector, KgrServer::Connector, KgrContextManager<KgrLocalContext> &);
        bool Attach(std::unique_ptr<KgrPipe>) override;
    
     private:
        SlokedSynchronized<SlokedScreenComponent &> &root;
        const Encoding &encoding;
        KgrServer::Connector cursorService;
        KgrServer::Connector renderService;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedScreenClient {
     public:
        class HandleClient {
         public:
            HandleClient(SlokedServiceClient &);
            bool NewMultiplexer(const std::string &) const;
            bool NewSplitter(const std::string &, Splitter::Direction) const;
            bool NewTabber(const std::string &) const;
            bool NewTextEditor(const std::string &, SlokedEditorDocumentSet::DocumentId) const;

         private:
            SlokedServiceClient &client;
        };

        class MultiplexerClient {
         public:
            MultiplexerClient(SlokedServiceClient &);
            std::optional<std::string> NewWindow(const std::string &, const TextPosition &, const TextPosition &) const;
            std::size_t GetWindowCount(const std::string &) const;
            std::optional<std::string> GetFocus(const std::string &) const;
            std::optional<bool> WindowHasFocus(const std::string &) const;
            bool SetFocus(const std::string &) const;
            bool Close(const std::string &) const;
            bool MoveWindow(const std::string &, const TextPosition &) const;
            bool ResizeWindow(const std::string &, const TextPosition &) const;

         private:
            SlokedServiceClient &client;
        };

        class SplitterClient {
         public:
            SplitterClient(SlokedServiceClient &);
            std::optional<std::string> NewWindow(const std::string &, const Splitter::Constraints &) const;
            std::optional<std::string> NewWindow(const std::string &, SlokedComponentWindow::Id, const Splitter::Constraints &) const;
            std::size_t GetWindowCount(const std::string &) const;
            std::optional<std::string> GetFocus(const std::string &) const;
            std::optional<bool> WindowHasFocus(const std::string &) const;
            bool SetFocus(const std::string &) const;
            bool Close(const std::string &) const;
            std::optional<std::string> MoveWindow(const std::string &, SlokedComponentWindow::Id) const;
            bool UpdateWindowConstraints(const std::string &, const Splitter::Constraints &) const;

         private:
            SlokedServiceClient &client;
        };

        class TabberClient {
         public:
            TabberClient(SlokedServiceClient &);
            std::optional<std::string> NewWindow(const std::string &) const;
            std::optional<std::string> NewWindow(const std::string &, SlokedComponentWindow::Id) const;
            std::size_t GetWindowCount(const std::string &) const;
            std::optional<std::string> GetFocus(const std::string &) const;
            std::optional<bool> WindowHasFocus(const std::string &) const;
            bool SetFocus(const std::string &) const;
            bool Close(const std::string &) const;
            std::optional<std::string> MoveWindow(const std::string &, SlokedComponentWindow::Id) const;

         private:
            SlokedServiceClient &client;
        };

        SlokedScreenClient(std::unique_ptr<KgrPipe>);

     private:
        SlokedServiceClient client;

     public:
        const HandleClient Handle;
        const MultiplexerClient Multiplexer;
        const SplitterClient Splitter;
        const TabberClient Tabber;
    };
}

#endif