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

#include "sloked/kgr/local/Server.h"

#include "sloked/core/Error.h"
#include "sloked/kgr/local/Pipe.h"
#include "sloked/sched/Pipeline.h"

namespace sloked {

    KgrLocalServer::KgrLocalServer()
        : lifetime(std::make_shared<SlokedStandardLifetime>()) {}

    KgrLocalServer::~KgrLocalServer() {
        this->lifetime->Close();
    }

    TaskResult<std::unique_ptr<KgrPipe>> KgrLocalServer::Connect(
        ServiceId srvId) {
        using Result = TaskResult<std::unique_ptr<KgrPipe>>;
        struct State {
            State(std::unique_ptr<KgrPipe> pipe) : pipe(std::move(pipe)) {}
            std::unique_ptr<KgrPipe> pipe;
        };

        if (this->services.count(srvId) == 0) {
            return Result::Reject(std::make_exception_ptr(SlokedError(
                "KgrServer: Unknown service #" + std::to_string(srvId))));
        }
        auto [clientPipe, servicePipe] = KgrLocalPipe::Make();
        static const SlokedAsyncTaskPipeline Pipeline(
            SlokedTaskPipelineStages::Map(
                [](const std::shared_ptr<State> &state)
                    -> std::unique_ptr<KgrPipe> {
                    return std::move(state->pipe);
                }),
            SlokedTaskPipelineStages::MapCancelled(
                [](const std::shared_ptr<State> &state)
                    -> std::unique_ptr<KgrPipe> {
                    throw SlokedError(
                        "LocalServer: Service cancelled attach operation");
                }));
        return Pipeline(
            std::make_shared<State>(std::move(clientPipe)),
            this->services.at(srvId)->Attach(std::move(servicePipe)),
            this->lifetime);
    }

    KgrLocalServer::Connector KgrLocalServer::GetConnector(ServiceId srvId) {
        return [this, srvId]() { return this->Connect(srvId); };
    }

    TaskResult<KgrLocalServer::ServiceId> KgrLocalServer::Register(
        std::unique_ptr<KgrService> service) {
        using Result = TaskResult<KgrLocalServer::ServiceId>;
        if (service == nullptr) {
            return Result::Reject(std::make_exception_ptr(
                SlokedError("KgrServer: Service can't be null")));
        }
        auto serviceId = this->serviceAllocator.Allocate();
        this->services.emplace(serviceId, std::move(service));
        return Result::Resolve(serviceId);
    }

    TaskResult<void> KgrLocalServer::Register(
        ServiceId serviceId, std::unique_ptr<KgrService> service) {
        using Result = TaskResult<void>;
        if (service == nullptr) {
            return Result::Reject(std::make_exception_ptr(
                SlokedError("KgrServer: Service can't be null")));
        }
        if (this->services.count(serviceId) == 0) {
            this->serviceAllocator.Set(serviceId, true);
            this->services.emplace(serviceId, std::move(service));
            return Result::Resolve();
        } else {
            return Result::Reject(std::make_exception_ptr(
                SlokedError("KgrLocalServer: Sevice #" +
                            std::to_string(serviceId) + " already allocated")));
        }
    }

    TaskResult<bool> KgrLocalServer::Registered(ServiceId serviceId) {
        return TaskResult<bool>::Resolve(this->services.count(serviceId) != 0);
    }

    TaskResult<void> KgrLocalServer::Deregister(ServiceId serviceId) {
        using Result = TaskResult<void>;
        if (this->services.count(serviceId)) {
            this->serviceAllocator.Set(serviceId, false);
            this->services.erase(serviceId);
            return Result::Resolve();
        } else {
            return Result::Reject(std::make_exception_ptr(SlokedError(
                "KgrServer: Unknown service #" + std::to_string(serviceId))));
        }
    }
}  // namespace sloked