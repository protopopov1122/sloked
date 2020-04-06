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
        : ctx(ctx), id(std::cref(id)) {}

    SlokedServiceContext::Response::Response(const Response &rsp)
        : ctx(rsp.ctx), id(KgrValue{}) {
        switch (rsp.id.index()) {
            case 0:
                this->id = KgrValue{std::get<0>(rsp.id)};
                break;

            case 1:
                this->id = std::get<1>(rsp.id);
                break;
        }
    }

    SlokedServiceContext::Response::Response(Response &&rsp)
        : ctx(std::move(rsp.ctx)), id(KgrValue{}) {
        switch (rsp.id.index()) {
            case 0:
                this->id = KgrValue{std::get<0>(rsp.id)};
                break;

            case 1:
                this->id = std::move(std::get<1>(rsp.id));
                break;
        }
    }

    SlokedServiceContext::Response &SlokedServiceContext::Response::operator=(
        const Response &rsp) {
        this->ctx = rsp.ctx;
        switch (rsp.id.index()) {
            case 0:
                this->id = KgrValue{std::get<0>(rsp.id)};
                break;

            case 1:
                this->id = std::get<1>(rsp.id);
                break;
        }
        return *this;
    }

    SlokedServiceContext::Response &SlokedServiceContext::Response::operator=(
        const Response &&rsp) {
        this->ctx = std::move(rsp.ctx);
        switch (rsp.id.index()) {
            case 0:
                this->id = KgrValue{std::get<0>(rsp.id)};
                break;

            case 1:
                this->id = std::move(std::get<1>(rsp.id));
                break;
        }
        return *this;
    }

    void SlokedServiceContext::Response::Result(KgrValue &&result) {
        switch (this->id.index()) {
            case 0:
                this->ctx.get().SendResponse(std::get<0>(this->id).get(),
                                             std::forward<KgrValue>(result));
                break;

            case 1:
                this->ctx.get().SendResponse(std::get<1>(this->id),
                                             std::forward<KgrValue>(result));
                break;
        }
    }

    void SlokedServiceContext::Response::Error(KgrValue &&error) {
        switch (this->id.index()) {
            case 0:
                this->ctx.get().SendError(std::get<0>(this->id).get(),
                                          std::forward<KgrValue>(error));
                break;

            case 1:
                this->ctx.get().SendError(std::get<1>(this->id),
                                          std::forward<KgrValue>(error));
                break;
        }
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
        } catch (const SlokedError &err) {
            this->HandleError(err, response.get());
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

    void SlokedServiceContext::HandleError(const SlokedError &err,
                                           Response *response) {
        throw err;
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

    SlokedServiceClient::Response::Response(bool has_result, KgrValue &&content)
        : has_result(has_result), content(std::forward<KgrValue>(content)) {}

    bool SlokedServiceClient::Response::HasResult() const {
        return this->has_result;
    }

    const KgrValue &SlokedServiceClient::Response::GetResult() const {
        if (this->has_result) {
            return this->content;
        } else {
            throw SlokedError("KgrServiceClient: Response contains error");
        }
    }

    const KgrValue &SlokedServiceClient::Response::GetError() const {
        if (!this->has_result) {
            return this->content;
        } else {
            throw SlokedError("KgrServiceClient: Response contains result");
        }
    }

    SlokedServiceClient::InvokeResult::~InvokeResult() {
        this->client.get().DropResult(*this);
    }

    TaskResult<SlokedServiceClient::Response>
        SlokedServiceClient::InvokeResult::Next() {
        std::unique_lock lock(this->mtx);
        TaskResultSupplier<Response> supplier;
        auto result = supplier.Result();
        if (this->pending.empty()) {
            this->awaiting.push(std::move(supplier));
        } else {
            auto res = std::move(this->pending.front());
            this->pending.pop();
            lock.unlock();
            supplier.SetResult(std::move(res));
        }
        return result;
    }

    SlokedServiceClient::InvokeResult::InvokeResult(SlokedServiceClient &client,
                                                    int64_t id)
        : client(client), id(id) {}

    void SlokedServiceClient::InvokeResult::Push(Response rsp) {
        std::unique_lock lock(this->mtx);
        if (this->awaiting.empty()) {
            this->pending.push(std::move(rsp));
        } else {
            auto supplier = std::move(this->awaiting.front());
            this->awaiting.pop();
            lock.unlock();
            supplier.SetResult(std::move(rsp));
        }
    }

    SlokedServiceClient::SlokedServiceClient(std::unique_ptr<KgrPipe> pipe)
        : mtx(std::make_unique<std::mutex>()), pipe(std::move(pipe)),
          nextId(0) {
        if (this->pipe == nullptr) {
            throw SlokedError("SlokedServiceClient: Pipe can't be null");
        } else {
            this->pipe->SetMessageListener([this] {
                while (this->pipe->Count() > 0) {
                    auto rsp = this->pipe->Read();
                    int64_t rspId = rsp.AsDictionary()["id"].AsInt();
                    std::unique_lock lock(*this->mtx);
                    if (this->active.count(rspId) == 0) {
                        continue;
                    }
                    auto result = this->active.at(rspId).lock();
                    if (result == nullptr) {
                        this->active.erase(rspId);
                        continue;
                    }
                    if (rsp.AsDictionary().Has("result")) {
                        result->Push(Response(
                            true, std::move(rsp.AsDictionary()["result"])));
                    } else {
                        result->Push(Response(
                            false, std::move(rsp.AsDictionary()["error"])));
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

    std::shared_ptr<SlokedServiceClient::InvokeResult>
        SlokedServiceClient::Invoke(const std::string &method,
                                    KgrValue &&params) {
        auto result = this->NewResult();
        this->pipe->Write(KgrDictionary{
            {"id", result->id}, {"method", method}, {"params", params}});
        return result;
    }

    void SlokedServiceClient::Close() {
        this->pipe->Close();
    }

    std::shared_ptr<SlokedServiceClient::InvokeResult>
        SlokedServiceClient::NewResult() {
        std::unique_lock lock(*this->mtx);
        std::shared_ptr<InvokeResult> result(
            new InvokeResult(*this, this->nextId++));
        this->active.insert_or_assign(result->id, result);
        return result;
    }

    void SlokedServiceClient::DropResult(InvokeResult &result) {
        std::unique_lock lock(*this->mtx);
        this->active.erase(result.id);
    }
}  // namespace sloked