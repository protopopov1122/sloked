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

#include "sloked/core/CharPreset.h"
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
                            const Encoding &, const SlokedCharPreset &,
                            KgrServer::Connector, KgrServer::Connector,
                            KgrServer::Connector,
                            KgrContextManager<KgrLocalContext> &);
        TaskResult<void> Attach(std::unique_ptr<KgrPipe>) override;

     private:
        SlokedMonitor<SlokedScreenComponent &> &root;
        const Encoding &encoding;
        const SlokedCharPreset &charPreset;
        KgrServer::Connector cursorService;
        KgrServer::Connector renderService;
        KgrServer::Connector notifyService;
        KgrContextManager<KgrLocalContext> &contextManager;
    };

    class SlokedScreenClient {
     public:
        class HandleClient {
         public:
            HandleClient(SlokedServiceClient &);
            TaskResult<bool> NewMultiplexer(const std::string &) const;
            TaskResult<bool> NewSplitter(const std::string &,
                                         Splitter::Direction) const;
            TaskResult<bool> NewTabber(const std::string &) const;
            TaskResult<bool> NewTextEditor(const std::string &,
                                           SlokedEditorDocumentSet::DocumentId,
                                           const std::string & = "") const;

         private:
            SlokedServiceClient &client;
        };

        class MultiplexerClient {
         public:
            MultiplexerClient(SlokedServiceClient &);
            TaskResult<std::optional<std::string>> NewWindow(
                const std::string &, const TextPosition &,
                const TextPosition &) const;
            TaskResult<std::size_t> GetWindowCount(const std::string &) const;
            TaskResult<std::optional<std::string>> GetFocus(
                const std::string &) const;
            TaskResult<std::optional<bool>> WindowHasFocus(
                const std::string &) const;
            TaskResult<bool> SetFocus(const std::string &) const;
            TaskResult<bool> Close(const std::string &) const;
            TaskResult<bool> MoveWindow(const std::string &,
                                        const TextPosition &) const;
            TaskResult<bool> ResizeWindow(const std::string &,
                                          const TextPosition &) const;

         private:
            SlokedServiceClient &client;
        };

        class SplitterClient {
         public:
            SplitterClient(SlokedServiceClient &);
            TaskResult<std::optional<std::string>> NewWindow(
                const std::string &, const Splitter::Constraints &) const;
            TaskResult<std::optional<std::string>> NewWindow(
                const std::string &, SlokedComponentWindow::Id,
                const Splitter::Constraints &) const;
            TaskResult<std::size_t> GetWindowCount(const std::string &) const;
            TaskResult<std::optional<std::string>> GetFocus(
                const std::string &) const;
            TaskResult<std::optional<bool>> WindowHasFocus(
                const std::string &) const;
            TaskResult<bool> SetFocus(const std::string &) const;
            TaskResult<bool> Close(const std::string &) const;
            TaskResult<std::optional<std::string>> MoveWindow(
                const std::string &, SlokedComponentWindow::Id) const;
            TaskResult<bool> UpdateWindowConstraints(
                const std::string &, const Splitter::Constraints &) const;

         private:
            SlokedServiceClient &client;
        };

        class TabberClient {
         public:
            TabberClient(SlokedServiceClient &);
            TaskResult<std::optional<std::string>> NewWindow(
                const std::string &) const;
            TaskResult<std::optional<std::string>> NewWindow(
                const std::string &, SlokedComponentWindow::Id) const;
            TaskResult<std::size_t> GetWindowCount(const std::string &) const;
            TaskResult<std::optional<std::string>> GetFocus(
                const std::string &) const;
            TaskResult<std::optional<bool>> WindowHasFocus(
                const std::string &) const;
            TaskResult<bool> SetFocus(const std::string &) const;
            TaskResult<bool> Close(const std::string &) const;
            TaskResult<std::optional<std::string>> MoveWindow(
                const std::string &, SlokedComponentWindow::Id) const;

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
}  // namespace sloked

#endif