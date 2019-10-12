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

#ifndef SLOKED_EDITOR_TABS_H_
#define SLOKED_EDITOR_TABS_H_

#include "sloked/screen/components/TabberComponent.h"
#include "sloked/kgr/Server.h"
#include "sloked/services/DocumentSet.h"
#include <map>

namespace sloked {

    class SlokedEditorTabs {
     public:
        class Tab {
         public:
            Tab(std::shared_ptr<SlokedComponentWindow>, SlokedDocumentSetClient, SlokedEditorTabs &);
            ~Tab();
            bool IsOpened() const;
            bool HasFocus() const;
            void SetFocus() const;
            SlokedComponentWindow::Id GetId() const;
            void Save();
            void Save(const std::string &);
            void Close();

         private:
            std::shared_ptr<SlokedComponentWindow> win;
            SlokedDocumentSetClient document;
            SlokedEditorTabs &tabs;
        };

        friend class Tab;

        SlokedEditorTabs(SlokedTabberComponent &, const Encoding &, KgrServer::Connector, KgrServer::Connector, KgrServer::Connector);
        ~SlokedEditorTabs();
        std::size_t GetTabCount();
        std::shared_ptr<Tab> GetTab(SlokedComponentWindow::Id);
        std::shared_ptr<Tab> GetFocus();
        std::shared_ptr<Tab> New(const std::string &, const std::string &);
        std::shared_ptr<Tab> Open(const std::string &, const std::string &, const std::string &);
        void Close();

     private:
        SlokedTabberComponent &tabber;
        const Encoding &encoding;
        KgrServer::Connector cursorService;
        KgrServer::Connector renderService;
        KgrServer::Connector documentService;
        std::map<SlokedComponentWindow::Id, std::shared_ptr<Tab>> documents;
    };
}

#endif