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

#include "sloked/services/CredentialMaster.h"

namespace sloked {

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

    class SlokedCredentialMasterServiceContext : public SlokedServiceContext {
     public:
        SlokedCredentialMasterServiceContext(
            std::unique_ptr<KgrPipe> pipe,
            SlokedCredentialMaster &credentialMaster)
            : SlokedServiceContext(std::move(pipe)),
              credentialMaster(credentialMaster) {
            this->BindMethod("newAccount",
                             &SlokedCredentialMasterServiceContext::NewAccount);
        }

     private:
        void NewAccount(const std::string &method, const KgrValue &params,
                        Response &rsp) {
            const auto &dict = params.AsDictionary();
            const auto &username = dict["id"].AsString();
            if (this->credentialMaster.Has(username)) {
                throw SlokedServiceError(
                    std::string{"CredentialMasterContext: Account \""} +
                    username + "\" already exists");
            }
            std::shared_ptr<SlokedCredentialMaster::Account> account;
            if (dict.Has("password")) {
                const auto &password = dict["password"].AsString();
                account = this->credentialMaster.New(username, password);
            } else {
                account = this->credentialMaster.New(username);
            }
            if (account == nullptr) {
                throw SlokedServiceError(
                    std::string{
                        "CredentialMasterContext: Error creating account \""} +
                    username + "\"");
            }
            if (dict.Has("restrictAccess")) {
                account->SetAccessRestrictions(
                    KgrToRestriction(dict["restrictAccess"].AsDictionary()));
            }
            if (dict.Has("restrictModification")) {
                account->SetModificationRestrictions(KgrToRestriction(
                    dict["restrictModification"].AsDictionary()));
            }
            rsp.Result(KgrDictionary{{"id", account->GetIdentifier()},
                                     {"password", account->GetPassword()}});
        }

        SlokedCredentialMaster &credentialMaster;
    };

    SlokedCredentialMasterService::SlokedCredentialMasterService(
        SlokedCredentialMaster &credentialMaster,
        KgrContextManager<KgrLocalContext> &contextManager)
        : credentialMaster(credentialMaster), contextManager(contextManager) {}

    TaskResult<void> SlokedCredentialMasterService::Attach(
        std::unique_ptr<KgrPipe> pipe) {
        TaskResultSupplier<void> supplier;
        supplier.Wrap([&] {
            auto ctx = std::make_unique<SlokedCredentialMasterServiceContext>(
                std::move(pipe), this->credentialMaster);
            this->contextManager.Attach(std::move(ctx));
        });
        return supplier.Result();
    }
}  // namespace sloked