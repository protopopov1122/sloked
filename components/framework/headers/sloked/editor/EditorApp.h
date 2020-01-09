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

#ifndef SLOKED_EDITOR_EDITORAPP_H_
#define SLOKED_EDITOR_EDITORAPP_H_

#include "sloked/core/Closeable.h"
#include "sloked/core/Semaphore.h"
#include "sloked/core/awaitable/Poll.h"
#include "sloked/core/CharWidth.h"
#include "sloked/sched/Scheduler.h"
#include "sloked/net/Socket.h"
#include "sloked/core/Crypto.h"
#include "sloked/security/Master.h"
#include "sloked/security/Slave.h"
#include "sloked/security/Authenticator.h"
#include "sloked/editor/EditorCore.h"
#include <atomic>
#include <variant>

namespace sloked {

    class SlokedEditorApp {
     public:
        class Crypto {
         public:
            Crypto(SlokedCrypto &);
            SlokedCrypto &GetEngine() const;
            bool HasCredentialMaster() const;
            SlokedCredentialMaster &GetCredentialMaster() const;
            bool HasCredentialSlave() const;
            SlokedCredentialSlave &GetCredentialSlave() const;
            bool HasAuthenticator() const;
            SlokedAuthenticatorFactory &GetAuthenticator() const;

            SlokedCredentialMaster &SetupCredentialMaster(SlokedCrypto::Key &);
            SlokedCredentialSlave &SetupCredentialSlave();
            SlokedAuthenticatorFactory &SetupAuthenticator(const std::string &);

         private:
            SlokedCrypto &crypto;
            std::variant<std::unique_ptr<SlokedCredentialMaster>, std::unique_ptr<SlokedCredentialSlave>> credentials;
            std::unique_ptr<SlokedAuthenticatorFactory> authenticator;
        };

        class Network {
         public:
            Network(SlokedEditorApp &, SlokedSocketFactory &);
            SlokedSocketFactory &GetEngine() const;
            void SetupCrypto(SlokedCrypto::Key &);

         private:
            SlokedEditorApp &app;
            SlokedSocketFactory &baseEngine;
            std::unique_ptr<SlokedSocketFactory> engine;
        };

        SlokedEditorApp();

        bool IsRunning() const;
        void Initialize(std::unique_ptr<SlokedIOPoll>, SlokedSocketFactory &);
        SlokedEditorApp::Crypto &InitializeCrypto(SlokedCrypto &);
        bool HasMasterServer() const;
        SlokedEditorMasterCore &GetMasterServer() const;
        SlokedEditorMasterCore &InitializeMasterServer(SlokedLogger &, SlokedMountableNamespace &, SlokedNamespaceMounter &);
        bool HasSlaveServer() const;
        void RequestStop();
        void WaitForStop();
        void Attach(SlokedCloseable &);

        SlokedCharWidth &GetCharWidth();
        SlokedSchedulerThread &GetScheduler();
        SlokedIOPoller &GetIO();
        Network &GetNetwork();
        Crypto &GetCrypto();

     private:
        std::atomic<bool> running;
        SlokedCloseablePool closeables;
        SlokedSemaphore termination;
        SlokedDefaultSchedulerThread sched;
        std::unique_ptr<SlokedIOPoll> ioPoll;
        std::unique_ptr<SlokedDefaultIOPollThread> ioPoller;
        std::unique_ptr<Network> network;
        std::unique_ptr<Crypto> crypto;
        std::variant<std::unique_ptr<SlokedEditorMasterCore>, std::unique_ptr<SlokedEditorSlaveCore>> server;
        SlokedCharWidth charWidth;
    };
}

#endif