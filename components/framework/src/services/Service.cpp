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

#include "sloked/services/Service.h"

#include "sloked/kgr/net/Config.h"

namespace sloked {

    SlokedServiceContext::Response::Response(SlokedServiceContext &ctx,
                                             const KgrValue &id)
        : ctx(ctx), id(id) {}

    SlokedServiceContext::Response::Response(const Response &rsp)
        : ctx(rsp.ctx), id(rsp.id) {}

    SlokedServiceContext::Response::Response(Response &&rsp)
        : ctx(std::move(rsp.ctx)), id(std::move(rsp.id)) {}

    SlokedServiceContext::Response &SlokedServiceContext::Response::operator=(
        const Response &rsp) {
        this->ctx = rsp.ctx;
        this->id = rsp.id;
        return *this;
    }

    SlokedServiceContext::Response &SlokedServiceContext::Response::operator=(
        const Response &&rsp) {
        this->ctx = std::move(rsp.ctx);
        this->id = std::move(rsp.id);
        return *this;
    }

    void SlokedServiceContext::Response::Result(KgrValue &&result) {
        this->ctx.get().SendResponse(this->id, std::forward<KgrValue>(result));
    }

    void SlokedServiceContext::Response::Error(KgrValue &&error) {
        this->ctx.get().SendError(this->id, std::forward<KgrValue>(error));
    }

    void SlokedServiceContext::Run() {
        std::unique_ptr<Response> response;
        try {
            if (!this->pipe->Empty()) {
                auto msg = this->pipe->Read();
                const auto &id = msg.AsDictionary()["id"];
                const std::string &method =
                    msg.AsDictionary()["method"].AsString();
                KgrValue None{};
                const auto &params = msg.AsDictionary().Has("params")
                                         ? msg.AsDictionary()["params"]
                                         : None;
                response = std::make_unique<Response>(*this, id);
                if (this->methods.count(method) != 0) {
                    this->methods.at(method)(method, params, *response);
                } else {
                    this->InvokeMethod(method, params, *response);
                }
            }
            if (this->eventLoop.HasPending()) {
                this->eventLoop.Run();
            }
        } catch (...) {
            this->HandleError(std::current_exception(), response.get());
        }
    }

    void SlokedServiceContext::SetActivationListener(
        std::function<void()> callback) {
        this->KgrLocalContext::SetActivationListener(callback);
        this->eventLoop.Notify(callback);
    }

    KgrServiceContext::State SlokedServiceContext::GetState() const {
        auto state = this->KgrLocalContext::GetState();
        if (state == State::Idle && this->eventLoop.HasPending()) {
            state = State::Pending;
        }
        return state;
    }

    void SlokedServiceContext::BindMethod(const std::string &method,
                                          MethodHandler handler) {
        this->methods.emplace(method, std::move(handler));
    }

    void SlokedServiceContext::InvokeMethod(const std::string &method,
                                            const KgrValue &, Response &) {
        throw SlokedError("Unimplemented method: " + method);
    }

    void SlokedServiceContext::HandleError(std::exception_ptr ex,
                                           Response *response) {
        try {
            std::rethrow_exception(ex);
        } catch (const SlokedServiceError &err) {
            if (response) {
                response->Error(err.what());
            }
        } catch (const SlokedError &err) {
            if (response) {
                response->Error(
                    std::string{"ServiceContext: Unhandled error \""} +
                    err.what() + "\"");
            }
            throw;
        } catch (...) {
            if (response) {
                response->Error("ServiceContext: Internal error");
            }
            throw;
        }
    }

    void SlokedServiceContext::SendResponse(const KgrValue &id,
                                            KgrValue &&msg) {
        this->pipe->Write(
            KgrDictionary{{"id", id}, {"result", std::forward<KgrValue>(msg)}});
    }

    void SlokedServiceContext::SendError(const KgrValue &id, KgrValue &&msg) {
        this->pipe->Write(
            KgrDictionary{{"id", id}, {"error", std::forward<KgrValue>(msg)}});
    }

    SlokedServiceClient::SlokedServiceClient(std::unique_ptr<KgrPipe> pipe)
        : pipe(std::move(pipe)) {
        if (this->pipe == nullptr) {
            throw SlokedError("SlokedServiceClient: Pipe can't be null");
        } else {
            this->pipe->SetMessageListener([this] {
                while (this->pipe->Count() > 0) {
                    auto rsp = this->pipe->Read();
                    int64_t rspId = rsp.AsDictionary()["id"].AsInt();
                    if (rsp.AsDictionary().Has("result")) {
                        this->responseBroker.Feed(
                            rspId,
                            SlokedNetResponseBroker::Response(
                                std::move(rsp.AsDictionary()["result"]), true));
                    } else {
                        this->responseBroker.Feed(
                            rspId,
                            SlokedNetResponseBroker::Response(
                                std::move(rsp.AsDictionary()["error"]), false));
                    }
                }
            });
        }
    }

    KgrPipe::Status SlokedServiceClient::GetStatus() const {
        if (this->pipe) {
            return this->pipe->GetStatus();
        } else {
            return KgrPipe::Status::Closed;
        }
    }

    std::shared_ptr<SlokedNetResponseBroker::Channel>
        SlokedServiceClient::Invoke(const std::string &method,
                                    KgrValue &&params) {
        auto result = this->responseBroker.OpenChannel();
        this->pipe->Write(KgrDictionary{
            {"id", result->GetID()}, {"method", method}, {"params", params}});
        return result;
    }

    void SlokedServiceClient::Close() {
        this->responseBroker.Close();
        this->pipe->Close();
    }
}  // namespace sloked