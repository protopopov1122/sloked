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

#include "sloked/services/Service.h"

using namespace std::chrono_literals;

namespace sloked {
    static std::chrono::system_clock Clock;
    static const auto DeferredCheckInterval = 10ms;

    SlokedServiceContext::Response::Response(SlokedServiceContext &ctx, const KgrValue &id)
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

    SlokedServiceContext::Response &SlokedServiceContext::Response::operator=(const Response &rsp) {
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
    
    SlokedServiceContext::Response &SlokedServiceContext::Response::operator=(const Response &&rsp) {
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
                this->ctx.get().SendResponse(std::get<0>(this->id).get(), std::forward<KgrValue>(result));
                break;

            case 1:
                this->ctx.get().SendResponse(std::get<1>(this->id), std::forward<KgrValue>(result));
                break;
        }
    }

    void SlokedServiceContext::Response::Error(KgrValue &&error) {
        switch (this->id.index()) {
            case 0:
                this->ctx.get().SendError(std::get<0>(this->id).get(), std::forward<KgrValue>(error));
                break;

            case 1:
                this->ctx.get().SendError(std::get<1>(this->id), std::forward<KgrValue>(error));
                break;
        }
    }

    void SlokedServiceContext::Run() {
        std::unique_ptr<Response> response;
        try {
            if (!this->pipe->Empty()) {
                auto msg = this->pipe->Read();
                const auto &id = msg.AsDictionary()["id"];
                const std::string &method = msg.AsDictionary()["method"].AsString();
                const auto &params = msg.AsDictionary()["params"];
                response = std::make_unique<Response>(*this, id);
                if (this->methods.count(method) != 0) {
                    this->methods.at(method)(method, params, *response);
                } else {
                    this->InvokeMethod(method, params, *response);
                }
            }
            if (std::chrono::duration_cast<std::chrono::milliseconds>(Clock.now() - this->lastDeferredCheck) < DeferredCheckInterval) {
                return;
            }
            for (auto it = this->deferred.begin(); it != this->deferred.end(); ++it) {
                if (it->first()) {
                    it->second();
                    this->deferred.erase(it);
                    break;
                }
            }
        } catch (const SlokedError &err) {
            this->HandleError(err, response.get());
        }
    }

    KgrServiceContext::State SlokedServiceContext::GetState() const {
        auto state = this->KgrLocalContext::GetState();
        if (state == State::Idle) {
            for (const auto &deferred : this->deferred) {
                if (deferred.first()) {
                    return State::Pending;
                }
            }
        }
        return state;
    }

    void SlokedServiceContext::BindMethod(const std::string &method, MethodHandler handler) {
        this->methods.emplace(method, std::move(handler));
    }

    void SlokedServiceContext::Defer(std::function<bool()> ready, std::function<void()> callback) {
        this->deferred.push_back(std::make_pair(std::move(ready), std::move(callback)));
    }

    void SlokedServiceContext::InvokeMethod(const std::string &method, const KgrValue &, Response &) {
        throw SlokedError("Unimplemented method: " + method);
    }

    void SlokedServiceContext::HandleError(const SlokedError &err, Response *response) {
        throw err;
    }

    void SlokedServiceContext::SendResponse(const KgrValue &id, KgrValue &&msg) {
        this->pipe->Write(KgrDictionary {
            { "id", id },
            { "result", std::forward<KgrValue>(msg) }
        });
    }

    void SlokedServiceContext::SendError(const KgrValue &id, KgrValue &&msg) {
        this->pipe->Write(KgrDictionary {
            { "id", id },
            { "error", std::forward<KgrValue>(msg) }
        });
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

    SlokedServiceClient::ResponseHandle::ResponseHandle(int64_t id, SlokedServiceClient &client)
        : id(id), client(client) {
        this->client.get().SetupHandle(id);
    }

    SlokedServiceClient::ResponseHandle::ResponseHandle(ResponseHandle &&rsp)
        : id(rsp.id), client(rsp.client) {
        rsp.id = -1;
    }
    
    SlokedServiceClient::ResponseHandle::~ResponseHandle() {
        this->client.get().ClearHandle(this->id);
    }

    void SlokedServiceClient::ResponseHandle::Drop() {
        this->client.get().Drop(this->id);
    }

    SlokedServiceClient::Response SlokedServiceClient::ResponseHandle::Get() {
        return this->client.get().Get(this->id);
    }

    std::optional<SlokedServiceClient::Response> SlokedServiceClient::ResponseHandle::GetOptional() {
        if (this->client.get().Has(this->id)) {
            return this->client.get().Get(this->id);
        } else {
            return {};
        }
    }

    bool SlokedServiceClient::ResponseHandle::Has() {
        return this->client.get().Has(this->id);
    }

    SlokedServiceClient::SlokedServiceClient(std::unique_ptr<KgrPipe> pipe)
        : mtx(std::make_unique<std::recursive_mutex>()), pipe(std::move(pipe)), nextId(0) {
        if (this->pipe == nullptr) {
            throw SlokedError("SlokedServiceClient: Pipe can't be null");
        }
    }

    KgrPipe::Status SlokedServiceClient::GetStatus() const {
        if (this->pipe) {
            return this->pipe->GetStatus();
        } else {
            return KgrPipe::Status::Closed;
        }
    }

    SlokedServiceClient::ResponseHandle SlokedServiceClient::Invoke(const std::string &method, KgrValue &&params) {
        int64_t id = this->nextId++;
        this->pipe->Write(KgrDictionary {
            { "id", id },
            { "method", method },
            { "params", params }
        });
        return ResponseHandle(id, *this);
    }

    void SlokedServiceClient::Close() {
        this->pipe->Close();
    }

    void SlokedServiceClient::SetupHandle(int64_t id) {
        std::unique_lock lock(*this->mtx);
        this->responses.emplace(id, std::queue<Response>{});
    }

    void SlokedServiceClient::ClearHandle(int64_t id) {
        std::unique_lock lock(*this->mtx);
        if (this->responses.count(id)) {
            this->responses.erase(id);
        }
    }

    bool SlokedServiceClient::Has(int64_t id) {
        this->ReceivePending();
        std::unique_lock lock(*this->mtx);
        return this->responses.count(id) != 0 &&
            !this->responses.at(id).empty();
    }

    SlokedServiceClient::Response SlokedServiceClient::Get(int64_t id) {
        while (!this->Has(id)) {
            this->ReceiveOne();
        }
        std::unique_lock lock(*this->mtx);
        auto rsp = std::move(this->responses.at(id).front());
        this->responses.at(id).pop();
        return rsp;
    }
    
    void SlokedServiceClient::Drop(int64_t id) {
        while (!this->Has(id)) {
            this->ReceiveOne();
        }
        std::unique_lock lock(*this->mtx);
        this->responses.at(id).pop();
    }

    void SlokedServiceClient::ReceiveOne() {
        auto rsp = this->pipe->ReadWait();
        int64_t rspId = rsp.AsDictionary()["id"].AsInt();
        std::unique_lock lock(*this->mtx);
        if (this->responses.count(rspId) == 0) {
            return;
        }
        if (rsp.AsDictionary().Has("result")) {
            this->responses[rspId].push(Response(true, std::move(rsp.AsDictionary()["result"])));
        } else {
            this->responses[rspId].push(Response(false, std::move(rsp.AsDictionary()["error"])));
        }
    }

    void SlokedServiceClient::ReceivePending() {
        while (this->pipe->Count() > 0) {
            this->ReceiveOne();
        }
    }
}