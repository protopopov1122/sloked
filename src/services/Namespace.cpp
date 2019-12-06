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