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

namespace sloked {

    class SlokedEditorStartup {
     public:
        struct RuntimeConfiguration {
            std::unique_ptr<SlokedCrypto::Key> masterKey;
        };

        SlokedEditorStartup(SlokedLogger &, SlokedMountableNamespace &, SlokedNamespaceMounter &, SlokedCrypto * = nullptr);
        RuntimeConfiguration Setup(SlokedEditorApp &, const KgrValue &);

     private:
        void SetupCrypto(SlokedEditorApp &, const KgrDictionary &, RuntimeConfiguration &);
        void SetupMasterAuth(SlokedEditorApp &, const KgrDictionary &, SlokedCrypto::Key &, const std::string &);
        void SetupSlaveAuth(SlokedEditorApp &, const KgrDictionary &, const std::string &);
        void SetupServer(SlokedEditorApp &, const KgrDictionary &, RuntimeConfiguration &);
        
        SlokedLogger &logger;
        SlokedMountableNamespace &root;
        SlokedNamespaceMounter &mounter;
        SlokedCrypto *cryptoEngine;
    };
}

#endif