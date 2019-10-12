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

#include "sloked/editor/Tabs.h"
#include "sloked/screen/widgets/TextEditor.h"
#include "sloked/core/Error.h"

namespace sloked {

    SlokedEditorTabs::Tab::Tab(std::shared_ptr<SlokedComponentWindow> win, SlokedDocumentSetClient document, SlokedEditorTabs &tabs)
        : win(win), document(std::move(document)), tabs(tabs) {}

    SlokedEditorTabs::Tab::~Tab() {
        if (this->IsOpened()) {
            this->Close();
        }
    }

    bool SlokedEditorTabs::Tab::IsOpened() const {
        return this->win != nullptr;
    }

    bool SlokedEditorTabs::Tab::HasFocus() const {
        if (this->win) {
            return this->win->HasFocus();
        } else {
            throw SlokedError("Tab already closed");
        }
    }

    void SlokedEditorTabs::Tab::SetFocus() const {
        if (this->win) {
            this->win->SetFocus();
        } else {
            throw SlokedError("Tab already closed");
        }
    }

    SlokedComponentWindow::Id SlokedEditorTabs::Tab::GetId() const {
        if (this->win) {
            return this->win->GetId();
        } else {
            throw SlokedError("Tab already closed");
        }
    }

    void SlokedEditorTabs::Tab::Save() {
        if (this->win) {
            this->document.Save();
        } else {
            throw SlokedError("Tab already closed");
        }
    }
    
    void SlokedEditorTabs::Tab::Save(const std::string &path) {
        if (this->win) {
            this->document.Save(path);
        } else {
            throw SlokedError("Tab already closed");
        }
    }

    void SlokedEditorTabs::Tab::Close() {
        if (this->win) {
            this->tabs.documents.erase(this->GetId());
            this->document.Close();
            this->win->Close();
            this->win = nullptr;
        } else {
            throw SlokedError("Tab already closed");
        }
    }

    SlokedEditorTabs::SlokedEditorTabs(SlokedTabberComponent &tabber, const Encoding &encoding, KgrServer::Connector cursorService, KgrServer::Connector renderService, KgrServer::Connector documentService)
        : tabber(tabber), encoding(encoding), cursorService(std::move(cursorService)), renderService(std::move(renderService)), documentService(std::move(documentService)) {}

    SlokedEditorTabs::~SlokedEditorTabs() {
        this->Close();
    }

    std::size_t SlokedEditorTabs::GetTabCount() {
        return this->tabber.GetWindowCount();
    }

    std::shared_ptr<SlokedEditorTabs::Tab> SlokedEditorTabs::GetTab(SlokedComponentWindow::Id id) {
        if (this->documents.count(id)) {
            return this->documents.at(id);
        } else {
            return nullptr;
        }
    }

    std::shared_ptr<SlokedEditorTabs::Tab> SlokedEditorTabs::GetFocus() {
        auto win = this->tabber.GetFocus();
        if (win) {
            return this->documents.at(win->GetId());
        } else {
            return nullptr;
        }
    }

    std::shared_ptr<SlokedEditorTabs::Tab> SlokedEditorTabs::New(const std::string &encoding, const std::string &newline) {
        SlokedDocumentSetClient document(this->documentService());
        auto documentId = document.New(encoding, newline);
        if (documentId.has_value()) {
            auto editor = std::make_unique<SlokedTextEditor>(this->encoding, this->cursorService(), this->renderService(), documentId.value());
            auto win = this->tabber.NewWindow();
            win->GetComponent().NewTextPane(std::move(editor));
            auto tab = std::make_shared<Tab>(win, std::move(document), *this);
            this->documents[tab->GetId()] = tab;
            return tab;
        } else {
            return nullptr;
        }
    }
    std::shared_ptr<SlokedEditorTabs::Tab> SlokedEditorTabs::Open(const std::string &path, const std::string &encoding, const std::string &newline) {
        SlokedDocumentSetClient document(this->documentService());
        auto documentId = document.Open(path, encoding, newline);
        if (documentId.has_value()) {
            auto editor = std::make_unique<SlokedTextEditor>(this->encoding, this->cursorService(), this->renderService(), documentId.value());
            auto win = this->tabber.NewWindow();
            win->GetComponent().NewTextPane(std::move(editor));
            auto tab = std::make_shared<Tab>(win, std::move(document), *this);
            this->documents[tab->GetId()] = tab;
            return tab;
        } else {
            return nullptr;
        }
    }

    void SlokedEditorTabs::Close() {
        for (auto it = this->documents.begin(); it != this->documents.end();) {
            auto current = it->second;
            ++it;
            current->Close();
        }
        this->documents.clear();
    }
}