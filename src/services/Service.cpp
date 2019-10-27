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

namespace sloked {
    SlokedServiceContext::Response::Response(SlokedServiceContext &ctx, const KgrValue &id)
        : ctx(ctx), id(id) {}

    void SlokedServiceContext::Response::Result(KgrValue &&result) {
        this->ctx.SendResponse(this->id, std::forward<KgrValue>(result));
    }

    void SlokedServiceContext::Response::Error(KgrValue &&error) {
        this->ctx.SendError(this->id, std::forward<KgrValue>(error));
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
        } catch (const SlokedError &err) {
            this->HandleError(err, response.get());
        }
    }

    void SlokedServiceContext::BindMethod(const std::string &method, MethodHandler handler) {
        this->methods.emplace(method, std::move(handler));
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

    SlokedServiceClient::ResponseHandle::ResponseHandle(int64_t id, std::map<int64_t, std::queue<Response>> &responses, std::function<void()> receiveOne, std::function<void()> receivePending)
        : id(id), responses(responses), receiveOne(std::move(receiveOne)), receivePending(std::move(receivePending)) {
        this->responses[this->id] = std::queue<Response>{};
    }
    
    SlokedServiceClient::ResponseHandle::~ResponseHandle() {
        this->responses.erase(this->id);
    }

    void SlokedServiceClient::ResponseHandle::Drop() {
        while (this->responses.at(this->id).empty()) {
            this->receiveOne();
        }
        this->responses.at(this->id).pop();
    }

    SlokedServiceClient::Response SlokedServiceClient::ResponseHandle::Get() {
        while (this->responses.at(this->id).empty()) {
            this->receiveOne();
        }
        auto rsp = this->responses.at(this->id).front();
        this->responses.at(this->id).pop();
        return rsp;
    }

    std::optional<SlokedServiceClient::Response> SlokedServiceClient::ResponseHandle::GetOptional() {
        this->receivePending();
        if (!this->responses.at(this->id).empty()) {
            auto rsp = this->responses.at(this->id).front();
            this->responses.at(this->id).pop();
            return rsp;
        } else {
            return {};
        }
    }

    SlokedServiceClient::SlokedServiceClient(std::unique_ptr<KgrPipe> pipe)
        : pipe(std::move(pipe)), nextId(0) {}

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
        return ResponseHandle(id, this->responses, [this]() { this->ReceiveOne(); }, [this]() { this->ReceivePending(); });
    }

    void SlokedServiceClient::Close() {
        this->pipe->Close();
    }

    void SlokedServiceClient::ReceiveOne() {
        auto rsp = this->pipe->ReadWait();
        int64_t rspId = rsp.AsDictionary()["id"].AsInt();
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