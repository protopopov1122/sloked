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

#include "sloked/editor/Startup.h"

namespace sloked {

    SlokedEditorStartup::SlokedEditorStartup(SlokedLogger &logger, SlokedMountableNamespace &root, SlokedNamespaceMounter &mounter, SlokedCrypto *crypto)
        : logger(logger), root(root), mounter(mounter), cryptoEngine(crypto) {}

    SlokedEditorStartup::RuntimeConfiguration SlokedEditorStartup::Setup(SlokedEditorApp &editor, const KgrValue &rawConfig) {
        RuntimeConfiguration runtimeConf;
        const auto &config = rawConfig.AsDictionary();
        if (config.Has("crypto") && this->cryptoEngine) {
            const auto &cryptoConfig = config["crypto"].AsDictionary();
            this->SetupCrypto(editor, cryptoConfig, runtimeConf);
        }
        this->SetupServer(editor, config["server"].AsDictionary(), runtimeConf);
        return runtimeConf;
    }

    void SlokedEditorStartup::SetupCrypto(SlokedEditorApp &editor, const KgrDictionary &cryptoConfig, RuntimeConfiguration &conf) {
        std::unique_ptr<SlokedCrypto::Key> masterKey;
        editor.InitializeCrypto(*this->cryptoEngine);
        masterKey = editor.GetCrypto().GetEngine().DeriveKey(cryptoConfig["masterPassword"].AsString(), cryptoConfig["salt"].AsString());
        if (cryptoConfig.Has("authentication")) {
            const auto &authConfig = cryptoConfig["authentication"].AsDictionary();
            if (authConfig.Has("master")) {
                const auto &masterConfig = authConfig["master"].AsDictionary();
                this->SetupMasterAuth(editor, masterConfig, *masterKey, cryptoConfig["salt"].AsString());
            } else if (authConfig.Has("slave")) {
                const auto &slaveConfig = authConfig["slave"].AsDictionary();
                this->SetupSlaveAuth(editor, slaveConfig, cryptoConfig["salt"].AsString());
            }
        }
        editor.GetNetwork().EncryptionLayer(editor.GetCrypto().GetEngine(), *masterKey);
        conf.masterKey = std::move(masterKey);
    }

    static std::unique_ptr<SlokedNamedRestrictions> KgrToRestriction(const KgrDictionary &config) {
        std::vector<std::string> content;
        bool whitelist = config.Has("whitelist") && config["whitelist"].AsBoolean();
        for (const auto &entry : config["content"].AsArray()) {
            content.push_back(entry.AsString());
        }
        if (whitelist) {
            return SlokedNamedWhitelist::Make(std::move(content));
        } else {
            return SlokedNamedBlacklist::Make(std::move(content));
        }
    }

    void SlokedEditorStartup::SetupMasterAuth(SlokedEditorApp &editor, const KgrDictionary &masterConfig, SlokedCrypto::Key &masterKey, const std::string &salt) {
        auto &authMaster = editor.GetCrypto().SetupCredentialMaster(masterKey);
        editor.GetCrypto().SetupAuthenticator(salt);
        if (masterConfig.Has("users")) {
            for (const auto &usr : masterConfig["users"].AsArray()) {
                const auto &userConfig = usr.AsDictionary();
                auto userAccount = authMaster.New(userConfig["id"].AsString()).lock();
                if (userConfig.Has("restrictAccess")) {
                    userAccount->SetAccessRestrictions(KgrToRestriction(userConfig["restrictAccess"].AsDictionary()));
                }
                if (userConfig.Has("restrictModification")) {
                    userAccount->SetModificationRestrictions(KgrToRestriction(userConfig["restrictModification"].AsDictionary()));
                }
            }
        }
        if (masterConfig.Has("defaultUser")) {
            auto userAccount = authMaster.EnableDefaultAccount(true).lock();
            const auto &userConfig = masterConfig["defaultUser"].AsDictionary();
            if (userConfig.Has("restrictAccess")) {
                userAccount->SetAccessRestrictions(KgrToRestriction(userConfig["restrictAccess"].AsDictionary()));
            }
            if (userConfig.Has("restrictModification")) {
                userAccount->SetModificationRestrictions(KgrToRestriction(userConfig["restrictModification"].AsDictionary()));
            }
        } else {
            authMaster.EnableDefaultAccount(false);
        }
    }

    void SlokedEditorStartup::SetupSlaveAuth(SlokedEditorApp &editor, const KgrDictionary &slaveConfig, const std::string &salt) {
        auto &authSlave = editor.GetCrypto().SetupCredentialSlave();
        editor.GetCrypto().SetupAuthenticator(salt);
        if (slaveConfig.Has("users")) {
            for (const auto &usr : slaveConfig["users"].AsArray()) {
                const auto &userConfig = usr.AsDictionary();
                authSlave.New(userConfig["id"].AsString(), userConfig["credentials"].AsString());
            }
        }
    }

    static SlokedSocketAddress KgrToSocketAddress(const KgrDictionary &conf) {
        return SlokedSocketAddress::Network {
            conf["host"].AsString(),
            static_cast<uint16_t>(conf["port"].AsInt())
        };
    }

    void SlokedEditorStartup::SetupServer(SlokedEditorApp &editor, const KgrDictionary &serverConfig, RuntimeConfiguration &conf) {
        if (serverConfig.Has("slave")) {
            const auto &slaveConfig = serverConfig["slave"].AsDictionary();
            auto socket = editor.GetNetwork().GetEngine().Connect(KgrToSocketAddress(slaveConfig["address"].AsDictionary()));
            editor.InitializeServer(std::move(socket));
        } else {
            editor.InitializeServer();
        }
        if (serverConfig.Has("netServer")) {
            auto address = KgrToSocketAddress(serverConfig["netServer"].AsDictionary());
            if (editor.HasCrypto()) {
                editor.GetServer().SpawnNetServer(editor.GetNetwork().GetEngine(), address, editor.GetIO(),
                    &editor.GetCrypto().GetCredentialMaster(), &editor.GetCrypto().GetAuthenticator());
            } else {
                editor.GetServer().SpawnNetServer(editor.GetNetwork().GetEngine(), address, editor.GetIO(),
                    nullptr, nullptr);
            }
        }
        if (serverConfig.Has("restrictAccess")) {
            editor.GetServer().GetRestrictions().SetAccessRestrictions(KgrToRestriction(serverConfig["restrictAccess"].AsDictionary()));
        }
        if (serverConfig.Has("restrictModification")) {
            editor.GetServer().GetRestrictions().SetModificationRestrictions(KgrToRestriction(serverConfig["restrictModification"].AsDictionary()));
        }
        if (serverConfig.Has("services")) {
            auto &serviceProvider = editor.InitializeServices(std::make_unique<SlokedServiceDependencyDefaultProvider>(this->logger, this->root, this->mounter, editor.GetCharWidth(), editor.GetServer().GetServer()));
            SlokedDefaultServicesFacade services(serviceProvider);
            for (const auto &service : serverConfig["services"].AsArray()) {
                editor.GetServer().GetServer().Register(service.AsString(), services.Build(service.AsString()));
            }
        }
    }
}