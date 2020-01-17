/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#ifndef SLOKED_EDITOR_STARTUP_H_
#define SLOKED_EDITOR_STARTUP_H_

#include "sloked/editor/EditorApp.h"
#include "sloked/namespace/Root.h"

namespace sloked {

    class SlokedEditorStartup : public SlokedCloseable, public SlokedEditorAppContainer {
     public:
        using EditorFactory = std::function<std::unique_ptr<SlokedEditorApp>()>;
        class Parameters {
         public:
            Parameters(SlokedLogger &, SlokedRootNamespaceFactory &);
            Parameters &SetTaggers(SlokedTextTaggerRegistry<int> &);
            Parameters &SetEditors(SlokedEditorStartup::EditorFactory);
            Parameters &SetCrypto(SlokedCrypto &);   

            friend class SlokedEditorStartup;
         private:
            SlokedLogger &logger;
            SlokedRootNamespaceFactory &root;
            SlokedTextTaggerRegistry<int> *taggers;
            SlokedEditorStartup::EditorFactory editors;
            SlokedCrypto *crypto;
        };
        SlokedEditorStartup(Parameters);
        void Spawn(const KgrValue &);
        void Close() final;
        void Setup(SlokedEditorApp &, const KgrValue &);

        bool Has(const std::string &) const final;
        SlokedEditorApp &Get(const std::string &) const final;
        void Enumerate(std::function<void(const std::string, SlokedEditorApp &)>) const final;
        SlokedEditorApp &Spawn(const std::string &, const KgrValue &) final;
        void Shutdown(const std::string &) final;

     private:
        void SetupCrypto(SlokedEditorApp &, const KgrDictionary &);
        void SetupMasterAuth(SlokedEditorApp &, const KgrDictionary &, const std::string &);
        void SetupSlaveAuth(SlokedEditorApp &, const KgrDictionary &, const std::string &);
        void SetupServer(SlokedEditorApp &, const KgrDictionary &);
        
        std::map<std::string, std::unique_ptr<SlokedEditorApp>> editors;
        SlokedLogger &logger;
        SlokedRootNamespaceFactory &namespaceFactory;
        SlokedTextTaggerRegistry<int> *baseTaggers;
        EditorFactory editorFactory;
        SlokedCrypto *cryptoEngine;
    };
}

#endif