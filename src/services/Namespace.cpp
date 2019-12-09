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

#include "sloked/services/Namespace.h"
#include "sloked/services/Service.h"

namespace sloked {

    class SlokedNamespaceServiceContext : public SlokedServiceContext {
     public:
        SlokedNamespaceServiceContext(std::unique_ptr<KgrPipe> pipe, SlokedNamespace &root)
            : SlokedServiceContext(std::move(pipe)), root(root) {
            this->BindMethod("list", &SlokedNamespaceServiceContext::List);
            this->BindMethod("walk", &SlokedNamespaceServiceContext::Walk);
            this->BindMethod("type", &SlokedNamespaceServiceContext::GetType);
            this->BindMethod("makeDir", &SlokedNamespaceServiceContext::MakeDir);
            this->BindMethod("makeFile", &SlokedNamespaceServiceContext::MakeFile);
            this->BindMethod("delete", &SlokedNamespaceServiceContext::Delete);
            this->BindMethod("rename", &SlokedNamespaceServiceContext::Rename);
            this->BindMethod("permissions", &SlokedNamespaceServiceContext::Permissions);
            this->BindMethod("uri", &SlokedNamespaceServiceContext::URI);
        }

     private:
        void List(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            if (this->root.HasObject(path)) {
                KgrArray res;
                this->root.Iterate(path, [&](const std::string &name, SlokedNamespaceObject::Type type) {
                    res.Append(KgrDictionary {
                        { "name", name },
                        {
                            "type", type == SlokedNamespaceObject::Type::File
                                ? "file"
                                : (type == SlokedNamespaceObject::Type::Directory
                                    ? "directory"
                                    : "none")

                        }
                    });
                });
                rsp.Result(std::move(res));
            } else {
                rsp.Result({});
            }
        }

        void Walk(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            if (this->root.HasObject(path)) {
                KgrArray res;
                this->root.Traverse(path, [&](const std::string &name, SlokedNamespaceObject::Type type) {
                    res.Append(KgrDictionary {
                        { "name", name },
                        {
                            "type", type == SlokedNamespaceObject::Type::File
                                ? "file"
                                : (type == SlokedNamespaceObject::Type::Directory
                                    ? "directory"
                                    : "none")

                        }
                    });
                });
                rsp.Result(std::move(res));
            } else {
                rsp.Result({});
            }
        }

        void GetType(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            auto obj = this->root.GetObject(path);
            if (obj) switch (obj->GetType()) {
                case SlokedNamespaceObject::Type::File:
                    rsp.Result("file");
                    break;

                case SlokedNamespaceObject::Type::Directory:
                    rsp.Result("directory");
                    break;

                default:
                    break;
            } else {
                rsp.Result("none");
            }
        }

        void MakeDir(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            auto handle = this->root.GetHandle(path);
            if (!handle->Exists()) {
                handle->MakeDir();
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void MakeFile(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            auto handle = this->root.GetHandle(path);
            if (!handle->Exists()) {
                handle->MakeFile();
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void Delete(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            auto handle = this->root.GetHandle(path);
            if (handle->Exists()) {
                handle->Delete();
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void Rename(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath from{params.AsDictionary()["from"].AsString()};
            SlokedPath to{params.AsDictionary()["to"].AsString()};
            auto handle = this->root.GetHandle(from);
            if (handle->Exists()) {
                handle->Rename(to);
                rsp.Result(true);
            } else {
                rsp.Result(false);
            }
        }

        void Permissions(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            auto handle = this->root.GetHandle(path);
            if (handle->Exists()) {
                KgrArray perms;
                if (handle->HasPermission(SlokedNamespacePermission::Read)) {
                    perms.Append("read");
                }
                if (handle->HasPermission(SlokedNamespacePermission::Write)) {
                    perms.Append("write");
                }
                rsp.Result(std::move(perms));
            } else {
                rsp.Result({});
            }
        }

        void URI(const std::string &method, const KgrValue &params, Response &rsp) {
            SlokedPath path{params.AsString()};
            auto handle = this->root.GetHandle(path);
            if (handle->Exists()) {
                auto uri = handle->ToURI();
                if (uri.has_value()) {
                    rsp.Result(uri.value());
                } else {
                    rsp.Result({});
                }
            } else {
                rsp.Result({});
            }
        }

        SlokedNamespace &root;
    };

    SlokedNamespaceService::SlokedNamespaceService(SlokedNamespace &root, KgrContextManager<KgrLocalContext> &contextManager)
        : root(root), contextManager(contextManager) {}

    bool SlokedNamespaceService::Attach(std::unique_ptr<KgrPipe> pipe) {
        auto ctx = std::make_unique<SlokedNamespaceServiceContext>(std::move(pipe), this->root);
        this->contextManager.Attach(std::move(ctx));
        return true;
    }
}