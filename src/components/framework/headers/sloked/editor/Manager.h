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
#include "sloked/editor/EditorContainer.h"
#include "sloked/namespace/Root.h"
#include "sloked/script/ScriptEngine.h"

namespace sloked {

    class SlokedEditorManager : public SlokedCloseable,
                                public SlokedEditorContainers {
     public:
        using EditorFactory =
            std::function<std::unique_ptr<SlokedEditorContainer>()>;
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
        void Setup(SlokedEditorContainer &, const KgrValue &);

        SlokedDefaultTextTaggerRegistry<SlokedEditorDocument::TagType>
            &GetBaseTaggers();
        const SlokedTextTaggerRegistry<SlokedEditorDocument::TagType>
            &GetBaseTaggers() const;
        bool HasEditorFactory() const;
        bool HasCrypto() const;
        bool HasCompression() const;
        bool HasScreen() const;
        SlokedEditorShutdown &GetTotalShutdown();
        SlokedRootNamespaceFactory &GetNamespaceFactory();

        bool Has(const std::string &) const final;
        SlokedEditorContainer &Get(const std::string &) const final;
        void Enumerate(std::function<void(const std::string,
                                          SlokedEditorContainer &)>) const final;
        SlokedEditorContainer &Spawn(const std::string &,
                                    const KgrValue &) final;
        void Shutdown(const std::string &) final;

     private:
        void SetupCrypto(SlokedEditorContainer &, const KgrDictionary &);
        void SetupMasterAuth(SlokedEditorContainer &, const KgrDictionary &,
                             const std::string &, SlokedCrypto::Cipher &);
        void SetupSlaveAuth(SlokedEditorContainer &, const KgrDictionary &,
                            const std::string &);
        void SetupServer(SlokedEditorContainer &, const KgrDictionary &);

        std::map<std::string, std::unique_ptr<SlokedEditorContainer>> editors;
        SlokedLogger &logger;
        SlokedRootNamespaceFactory &namespaceFactory;
        SlokedDefaultTextTaggerRegistry<SlokedEditorDocument::TagType>
            baseTaggers;
        EditorFactory editorFactory;
        SlokedCrypto *cryptoEngine;
        SlokedCompression *compression;
        SlokedScreenProviderFactory *screenProviders;
        SlokedDefaultEditorShutdown shutdown;
    };
}  // namespace sloked

#endif