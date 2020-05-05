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

#include "sloked/editor/Manager.h"

namespace sloked {

    SlokedEditorManager::Parameters::Parameters(
        SlokedLogger &logger, SlokedRootNamespaceFactory &root)
        : logger(logger), root(root), editors{nullptr}, crypto{nullptr},
          compression{nullptr}, screenProviders{nullptr} {}

    SlokedEditorManager::Parameters &
        SlokedEditorManager::Parameters::SetEditors(
            SlokedEditorManager::EditorFactory editors) {
        this->editors = std::move(editors);
        return *this;
    }

    SlokedEditorManager::Parameters &SlokedEditorManager::Parameters::SetCrypto(
        SlokedCrypto &crypto) {
        this->crypto = std::addressof(crypto);
        return *this;
    }

    SlokedEditorManager::Parameters &
        SlokedEditorManager::Parameters::SetComresssion(
            SlokedCompression &compression) {
        this->compression = std::addressof(compression);
        return *this;
    }

    SlokedEditorManager::Parameters &
        SlokedEditorManager::Parameters::SetScreenProviders(
            SlokedScreenProviderFactory &provider) {
        this->screenProviders = std::addressof(provider);
        return *this;
    }

    SlokedEditorManager::SlokedEditorManager(Parameters prms)
        : logger(prms.logger), namespaceFactory(prms.root),
          editorFactory(std::move(prms.editors)), cryptoEngine(prms.crypto),
          compression(prms.compression), screenProviders(prms.screenProviders) {
    }

    void SlokedEditorManager::Spawn(const KgrValue &config) {
        const auto &editors = config.AsDictionary();
        for (const auto &kv : editors) {
            this->Spawn(kv.first, kv.second);
        }
    }

    void SlokedEditorManager::Close() {
        for (auto it = this->editors.rbegin(); it != this->editors.rend();
             ++it) {
            it->second->Close();
        }
    }

    void SlokedEditorManager::Setup(SlokedEditorContainer &editor,
                                    const KgrValue &rawConfig) {
        const auto &config = rawConfig.AsDictionary();
        if (config.Has("crypto") && this->cryptoEngine) {
            const auto &cryptoConfig = config["crypto"].AsDictionary();
            this->SetupCrypto(editor, cryptoConfig);
        }
        if (config.Has("network")) {
            const auto &networkConfig = config["network"].AsDictionary();
            if (networkConfig.Has("compression") &&
                networkConfig["compression"].AsBoolean() &&
                this->compression != nullptr) {
                editor.GetNetwork().CompressionLayer(*this->compression);
            }
            if (networkConfig.Has("buffering")) {
                auto bufferingTimeout = networkConfig["buffering"].AsInt();
                editor.GetNetwork().BufferingLayer(
                    std::chrono::milliseconds(bufferingTimeout),
                    editor.GetScheduler());
            }
        }
        if (config.Has("parameters")) {
            const auto &parameters = config["parameters"].AsDictionary();
            if (parameters.Has("tabWidth")) {
                auto width = parameters["tabWidth"].AsInt();
                editor.GetCharPreset().SetTabWidth(width);
            }
        }
        this->SetupServer(editor, config["server"].AsDictionary());
    }

    SlokedDefaultTextTaggerRegistry<SlokedEditorDocument::TagType>
        &SlokedEditorManager::GetBaseTaggers() {
        return this->baseTaggers;
    }

    const SlokedTextTaggerRegistry<SlokedEditorDocument::TagType>
        &SlokedEditorManager::GetBaseTaggers() const {
        return this->baseTaggers;
    }

    bool SlokedEditorManager::HasEditorFactory() const {
        return this->editorFactory != nullptr;
    }

    bool SlokedEditorManager::HasCrypto() const {
        return this->cryptoEngine != nullptr;
    }

    bool SlokedEditorManager::HasCompression() const {
        return this->compression != nullptr;
    }

    bool SlokedEditorManager::HasScreen() const {
        return this->screenProviders != nullptr;
    }

    SlokedEditorShutdown &SlokedEditorManager::GetTotalShutdown() {
        return this->shutdown;
    }

    SlokedRootNamespaceFactory &SlokedEditorManager::GetNamespaceFactory() {
        return this->namespaceFactory;
    }

    bool SlokedEditorManager::Has(const std::string &key) const {
        return this->editors.count(key) != 0;
    }

    SlokedEditorContainer &SlokedEditorManager::Get(
        const std::string &key) const {
        if (this->editors.count(key) != 0) {
            return *this->editors.at(key);
        } else {
            throw SlokedError("EditorStartup: Editor \'" + key +
                              "\' is not defined");
        }
    }

    void SlokedEditorManager::Enumerate(
        std::function<void(const std::string, SlokedEditorContainer &)> callback)
        const {
        for (const auto &kv : this->editors) {
            callback(kv.first, *kv.second);
        }
    }

    SlokedEditorContainer &SlokedEditorManager::Spawn(const std::string &key,
                                                     const KgrValue &config) {
        if (this->editorFactory == nullptr) {
            throw SlokedError("EditorStartup: editor factory not defined");
        }
        if (this->editors.count(key) == 0) {
            auto editor = this->editorFactory();
            this->Setup(*editor, config);
            editor->Start();
            this->editors.emplace(key, std::move(editor));
            return *this->editors.at(key);
        } else {
            throw SlokedError("EditorStartup: Editor \'" + key +
                              "\' is already defined");
        }
    }

    void SlokedEditorManager::Shutdown(const std::string &key) {
        if (this->editors.count(key) != 0) {
            auto &editor = *this->editors.at(key);
            editor.Close();
            this->editors.erase(key);
        } else {
            throw SlokedError("EditorStartup: Editor \'" + key +
                              "\' is not defined");
        }
    }

    void SlokedEditorManager::SetupCrypto(SlokedEditorContainer &editor,
                                          const KgrDictionary &cryptoConfig) {
        std::unique_ptr<SlokedCrypto::Key> defaultKey;
        editor.InitializeCrypto(*this->cryptoEngine);
        auto cipher = editor.GetCrypto().GetEngine().NewCipher();
        if (cryptoConfig.Has("defaultKey")) {
            defaultKey = editor.GetCrypto().GetEngine().DeriveKey(
                cipher->Parameters().KeySize,
                cryptoConfig["defaultKey"]
                    .AsDictionary()["password"]
                    .AsString(),
                cryptoConfig["salt"].AsString());
        }
        if (cryptoConfig.Has("authentication")) {
            const auto &authConfig =
                cryptoConfig["authentication"].AsDictionary();
            if (authConfig.Has("master")) {
                const auto &masterConfig = authConfig["master"].AsDictionary();
                this->SetupMasterAuth(editor, masterConfig,
                                      cryptoConfig["salt"].AsString(), *cipher);
            } else if (authConfig.Has("slave")) {
                const auto &slaveConfig = authConfig["slave"].AsDictionary();
                this->SetupSlaveAuth(editor, slaveConfig,
                                     cryptoConfig["salt"].AsString());
            }
        }
        editor.GetNetwork().EncryptionLayer(editor.GetCrypto().GetEngine(),
                                            std::move(defaultKey));
    }

    static std::unique_ptr<SlokedNamedRestrictions> KgrToRestriction(
        const KgrDictionary &config) {
        std::vector<SlokedPath> content;
        bool whitelist =
            config.Has("whitelist") && config["whitelist"].AsBoolean();
        for (const auto &entry : config["content"].AsArray()) {
            content.push_back({entry.AsString()});
        }
        if (whitelist) {
            return SlokedNamedWhitelist::Make(std::move(content));
        } else {
            return SlokedNamedBlacklist::Make(std::move(content));
        }
    }

    void SlokedEditorManager::SetupMasterAuth(SlokedEditorContainer &editor,
                                              const KgrDictionary &masterConfig,
                                              const std::string &salt,
                                              SlokedCrypto::Cipher &cipher) {
        auto masterKey = editor.GetCrypto().GetEngine().DeriveKey(
            cipher.Parameters().KeySize,
            masterConfig["masterPassword"].AsString(),
            masterConfig["salt"].AsString());
        auto &authMaster = editor.GetCrypto().SetupCredentialMaster(*masterKey);
        editor.Attach(SlokedTypedDataHandle<SlokedCrypto::Key>::Wrap(
            std::move(masterKey)));
        editor.GetCrypto().SetupAuthenticator(salt);
        if (masterConfig.Has("users")) {
            for (const auto &usr : masterConfig["users"].AsArray()) {
                const auto &userConfig = usr.AsDictionary();
                auto userAccount =
                    userConfig.Has("password")
                        ? authMaster.New(userConfig["id"].AsString(),
                                         userConfig["password"].AsString())
                        : authMaster.New(userConfig["id"].AsString());
                if (userConfig.Has("restrictAccess")) {
                    userAccount->SetAccessRestrictions(KgrToRestriction(
                        userConfig["restrictAccess"].AsDictionary()));
                }
                if (userConfig.Has("restrictModification")) {
                    userAccount->SetModificationRestrictions(KgrToRestriction(
                        userConfig["restrictModification"].AsDictionary()));
                }
            }
        }
        if (masterConfig.Has("defaultUser")) {
            auto userAccount = authMaster.EnableDefaultAccount(true);
            const auto &userConfig = masterConfig["defaultUser"].AsDictionary();
            if (userConfig.Has("restrictAccess")) {
                userAccount->SetAccessRestrictions(KgrToRestriction(
                    userConfig["restrictAccess"].AsDictionary()));
            }
            if (userConfig.Has("restrictModification")) {
                userAccount->SetModificationRestrictions(KgrToRestriction(
                    userConfig["restrictModification"].AsDictionary()));
            }
        } else {
            authMaster.EnableDefaultAccount(false);
        }
    }

    void SlokedEditorManager::SetupSlaveAuth(SlokedEditorContainer &editor,
                                             const KgrDictionary &slaveConfig,
                                             const std::string &salt) {
        auto &authSlave = editor.GetCrypto().SetupCredentialSlave();
        editor.GetCrypto().SetupAuthenticator(salt);
        if (slaveConfig.Has("users")) {
            for (const auto &usr : slaveConfig["users"].AsArray()) {
                const auto &userConfig = usr.AsDictionary();
                authSlave.New(userConfig["id"].AsString(),
                              userConfig["password"].AsString());
            }
        }
    }

    static SlokedSocketAddress KgrToSocketAddress(const KgrDictionary &conf) {
        return SlokedSocketAddress::Network{
            conf["host"].AsString(),
            static_cast<uint16_t>(conf["port"].AsInt())};
    }

    void SlokedEditorManager::SetupServer(SlokedEditorContainer &editor,
                                          const KgrDictionary &serverConfig) {
        if (serverConfig.Has("slave")) {
            const auto &slaveConfig = serverConfig["slave"].AsDictionary();
            auto socket = editor.GetNetwork().GetEngine().Connect(
                KgrToSocketAddress(slaveConfig["address"].AsDictionary()));
            editor.InitializeServer(std::move(socket));
            if (slaveConfig.Has("authorize")) {
                editor.GetServer().AsRemoteServer().Authorize(
                    slaveConfig["authorize"].AsString());
            }
        } else {
            editor.InitializeServer();
        }
        if (serverConfig.Has("netServer")) {
            auto address =
                KgrToSocketAddress(serverConfig["netServer"].AsDictionary());
            if (editor.HasCrypto()) {
                editor.GetServer().SpawnNetServer(
                    editor.GetNetwork().GetEngine(), editor.GetScheduler(),
                    address, editor.GetIO(),
                    &editor.GetCrypto().GetCredentialMaster(),
                    &editor.GetCrypto().GetAuthenticator());
            } else {
                editor.GetServer().SpawnNetServer(
                    editor.GetNetwork().GetEngine(), editor.GetScheduler(),
                    address, editor.GetIO(), nullptr, nullptr);
            }
        }
        if (serverConfig.Has("restrictAccess")) {
            editor.GetServer().GetRestrictions().SetAccessRestrictions(
                KgrToRestriction(
                    serverConfig["restrictAccess"].AsDictionary()));
        }
        if (serverConfig.Has("restrictModification")) {
            editor.GetServer().GetRestrictions().SetModificationRestrictions(
                KgrToRestriction(
                    serverConfig["restrictModification"].AsDictionary()));
        }
        if (serverConfig.Has("services")) {
            const auto &serviceConfig = serverConfig["services"].AsDictionary();
            auto &serviceProvider = editor.InitializeServiceProvider(
                std::make_unique<SlokedServiceDependencyDefaultProvider>(
                    this->logger, this->namespaceFactory.Build(),
                    editor.GetCharPreset(), editor.GetServer().GetServer(),
                    editor.GetContextManager(),
                    editor.HasCrypto() &&
                            editor.GetCrypto().HasCredentialMaster()
                        ? &editor.GetCrypto().GetCredentialMaster()
                        : nullptr,
                        this->shutdown,
                    &this->baseTaggers));
            if (serviceConfig.Has("root")) {
                serviceProvider.GetNamespace().GetRoot().Mount(
                    SlokedPath{"/"},
                    serviceProvider.GetNamespace().GetMounter().Mount(
                        SlokedUri::Parse(serviceConfig["root"].AsString())));
            }
            SlokedDefaultServicesFacade services(serviceProvider);
            for (const auto &service : serviceConfig["endpoints"].AsArray()) {
                editor.GetServer()
                    .GetServer()
                    .Register({service.AsString()},
                              services.Build(service.AsString()))
                    .Wait();
            }
        }
        if (serverConfig.Has("screen")) {
            if (this->screenProviders != nullptr) {
                auto uri = SlokedUri::Parse(serverConfig["screen"].AsString());
                editor.InitializeScreen(*this->screenProviders, uri);
            } else {
                throw SlokedError("Startup: Screen providers are not defined");
            }
        }
    }
}  // namespace sloked