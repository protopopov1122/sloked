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

#include "sloked/editor/EditorDocuments.h"
#include "sloked/screen/components/TabberComponent.h"
#include "sloked/kgr/Server.h"
#include "sloked/services/DocumentSet.h"
#include <map>

namespace sloked {

    class SlokedEditorTabs : public SlokedEditorDocuments {
     public:
        class Tab : public SlokedEditorDocuments::DocumentWindow {
         public:
            Tab(std::shared_ptr<SlokedComponentWindow>, SlokedDocumentSetClient, SlokedEditorTabs &);
            virtual ~Tab();
            bool IsOpened() const override;
            bool HasFocus() const override;
            void SetFocus() const override;
            std::optional<std::string> GetPath() override;
            SlokedComponentWindow::Id GetId() const override;
            void Save() override;
            void Save(const std::string &) override;
            void Close() override;

         private:
            std::shared_ptr<SlokedComponentWindow> win;
            SlokedDocumentSetClient document;
            SlokedEditorTabs &tabs;
        };

        friend class Tab;

        SlokedEditorTabs(SlokedTabberComponent &, const Encoding &, KgrServer::Connector, KgrServer::Connector, KgrServer::Connector);
        ~SlokedEditorTabs();
        std::size_t GetWindowCount() override;
        std::shared_ptr<DocumentWindow> GetWindow(SlokedComponentWindow::Id) override;
        std::shared_ptr<DocumentWindow> GetFocus() override;
        std::shared_ptr<DocumentWindow> New(const std::string &, const std::string &) override;
        std::shared_ptr<DocumentWindow> Open(const std::string &, const std::string &, const std::string &) override;
        void CloseAll() override;

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