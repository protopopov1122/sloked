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

#ifndef SLOKED_EDITOR_MANAGER_H_
#define SLOKED_EDITOR_MANAGER_H_

#include "sloked/core/Compression.h"
#include "sloked/editor/Configuration.h"
#include "sloked/editor/EditorInstance.h"
#include "sloked/namespace/Root.h"
#include "sloked/script/ScriptEngine.h"

namespace sloked {

    class SlokedEditorManager : public SlokedCloseable,
                                public SlokedEditorInstanceContainer {
     public:
        using EditorFactory =
            std::function<std::unique_ptr<SlokedEditorInstance>()>;
        class Parameters {
         public:
            Parameters(SlokedLogger &, SlokedRootNamespaceFactory &);
            Parameters &SetEditors(SlokedEditorManager::EditorFactory);
            Parameters &SetCrypto(SlokedCrypto &);
            Parameters &SetComresssion(SlokedCompression &);
            Parameters &SetScreenProviders(SlokedScreenProviderFactory &);

            friend class SlokedEditorManager;

         private:
            SlokedLogger &logger;
            SlokedRootNamespaceFactory &root;
            SlokedEditorManager::EditorFactory editors;
            SlokedCrypto *crypto;
            SlokedCompression *compression;
            SlokedScreenProviderFactory *screenProviders;
        };
        SlokedEditorManager(Parameters);
        void Spawn(const KgrValue &);
        void Close() final;
        void Setup(SlokedEditorInstance &, const KgrValue &);

        SlokedDefaultTextTaggerRegistry<SlokedEditorDocument::TagType>
            &GetBaseTaggers();
        const SlokedTextTaggerRegistry<SlokedEditorDocument::TagType>
            &GetBaseTaggers() const;
        bool HasEditorFactory() const;
        bool HasCrypto() const;
        bool HasCompression() const;
        bool HasScreen() const;

        bool Has(const std::string &) const final;
        SlokedEditorInstance &Get(const std::string &) const final;
        void Enumerate(std::function<void(const std::string,
                                          SlokedEditorInstance &)>) const final;
        SlokedEditorInstance &Spawn(const std::string &,
                                    const KgrValue &) final;
        void Shutdown(const std::string &) final;

     private:
        void SetupCrypto(SlokedEditorInstance &, const KgrDictionary &);
        void SetupMasterAuth(SlokedEditorInstance &, const KgrDictionary &,
                             const std::string &, SlokedCrypto::Cipher &);
        void SetupSlaveAuth(SlokedEditorInstance &, const KgrDictionary &,
                            const std::string &);
        void SetupServer(SlokedEditorInstance &, const KgrDictionary &);

        std::map<std::string, std::unique_ptr<SlokedEditorInstance>> editors;
        SlokedLogger &logger;
        SlokedRootNamespaceFactory &namespaceFactory;
        SlokedDefaultTextTaggerRegistry<SlokedEditorDocument::TagType>
            baseTaggers;
        EditorFactory editorFactory;
        SlokedCrypto *cryptoEngine;
        SlokedCompression *compression;
        SlokedScreenProviderFactory *screenProviders;
    };
}  // namespace sloked

#endif