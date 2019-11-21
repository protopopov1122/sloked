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

#include "sloked/kgr/net/Interface.h"
#include "sloked/core/Error.h"
#include "sloked/core/Locale.h"
#include "sloked/kgr/Serialize.h"
#include <chrono>

using namespace std::chrono_literals;

namespace sloked {

    KgrNetInterface::Response::Response(const KgrValue &value, bool success)
        : content(value), result(success) {}

    bool KgrNetInterface::Response::HasResult() const {
        return this->result;
    }

    const KgrValue &KgrNetInterface::Response::GetResult() const {
        if (this->result) {
            return this->content;
        } else {
            throw SlokedError("KgrNetInterface: Result not available");
        }
    }

    const KgrValue &KgrNetInterface::Response::GetError() const {
        if (!this->result) {
            return this->content;
        } else {
            throw SlokedError("KgrNetInterface: Error not available");
        }
    }

    KgrNetInterface::ResponseHandle::ResponseHandle(KgrNetInterface &net, int64_t id)
        : net(&net), id(id) {
        std::unique_lock<std::mutex> lock(this->net->response_mtx);
        this->net->responses[this->id] = {};
    }

    KgrNetInterface::ResponseHandle::~ResponseHandle() {
        std::unique_lock<std::mutex> lock(this->net->response_mtx);
        this->net->responses.erase(this->id);
    }

    bool KgrNetInterface::ResponseHandle::HasResponse() const {
        std::unique_lock<std::mutex> lock(this->net->response_mtx);
        return !this->net->responses.at(this->id).empty();
    }

    bool KgrNetInterface::ResponseHandle::WaitResponse(long timeout) const {
        std::unique_lock<std::mutex> lock(this->net->response_mtx);
        this->net->response_cv.wait_for(lock, timeout * 1ms);
        return !this->net->responses.at(this->id).empty();
    }

    KgrNetInterface::Response KgrNetInterface::ResponseHandle::GetResponse() const {
        std::unique_lock<std::mutex> lock(this->net->response_mtx);
        auto &queue = this->net->responses.at(this->id);
        if (!queue.empty()) {
            auto res = std::move(queue.front());
            queue.pop();
            return res;
        } else {
            throw SlokedError("KgrNetResponse: No response");
        }
    }

    KgrNetInterface::Responder::Responder(KgrNetInterface &net, int64_t id)
        : net(net), id(id) {}

    void KgrNetInterface::Responder::Result(const KgrValue &value) {
        this->net.Write(KgrDictionary {
            { "action", "response" },
            { "id", this->id },
            { "result", value }
        });
    }

    void KgrNetInterface::Responder::Error(const KgrValue &value) {
        this->net.Write(KgrDictionary {
            { "action", "response" },
            { "id", this->id },
            { "error", value }
        });
    }

    KgrNetInterface::KgrNetInterface(std::unique_ptr<SlokedSocket> socket)
        : socket(std::move(socket)), nextId(0) {}

    bool KgrNetInterface::Wait(long timeout) const {
        return this->socket->Wait(timeout);
    }

    bool KgrNetInterface::Valid() const {
        return this->socket->Valid();
    }

    void KgrNetInterface::Receive() {
        if (this->socket->Available() > 0) {
            KgrJsonSerializer serializer;
            EncodingConverter conv(Encoding::Utf8, SlokedLocale::SystemEncoding());
            auto data = this->socket->Read(socket->Available());
            this->buffer.insert(this->buffer.end(), data.begin(), data.end());
            std::size_t start = 0;
            std::size_t end;
            while ((end = this->buffer.find('\0')) != std::string::npos) {
                std::string message = this->buffer.substr(start, end);
                this->buffer.erase(start, end + 1);
                try {
                    this->incoming.push(serializer.Deserialize(conv.Convert(message)));
                } catch (const SlokedError &err) {
                    // Ignoring malformed requests
                }
            }
        }
    }

    void KgrNetInterface::Process(std::size_t count) {
        bool allMessages = count == 0;
        while (!this->incoming.empty() && (allMessages || count--)) {
            try {
                auto msg = std::move(this->incoming.front());
                this->incoming.pop();
                const auto &action = msg.AsDictionary()["action"].AsString();
                if (action == "invoke") {
                    this->ActionInvoke(msg);
                } else if (action == "response") {
                    this->ActionResponse(msg);
                } else if (action == "close") {
                    this->ActionClose(msg);
                }
            } catch (const SlokedError &err) {
                // Ignoring malformed requests
            }
        }
    }

    KgrNetInterface::ResponseHandle KgrNetInterface::Invoke(const std::string &method, const KgrValue &params) {
        int64_t id = this->nextId++;
        this->Write(KgrDictionary {
            { "action", "invoke" },
            { "id", id },
            { "method", method },
            { "params", params }
        });
        return KgrNetInterface::ResponseHandle(*this, id);
    }

    void KgrNetInterface::Close() {
        this->Write(KgrDictionary {
            { "action", "close" }
        });
        this->socket->Close();
        this->buffer.clear();
        this->incoming = {};
        std::unique_lock<std::mutex> lock(this->response_mtx);
        this->responses.clear();
        this->response_cv.notify_all();
    }

    void KgrNetInterface::BindMethod(const std::string &method, std::function<void(const std::string &, const KgrValue &, Responder &)> handler) {
        this->methods[method] = handler;
    }

    std::unique_ptr<SlokedSocketAwaitable> KgrNetInterface::Awaitable() const {
        return this->socket->Awaitable();
    }

    void KgrNetInterface::InvokeMethod(const std::string &method, const KgrValue &params, Responder &rsp) {
        rsp.Error("Unknown method: " + method);
    }

    void KgrNetInterface::Write(const KgrValue &msg) {
        KgrJsonSerializer serializer;
        EncodingConverter conv(SlokedLocale::SystemEncoding(), Encoding::Utf8);
        auto raw = conv.Convert(serializer.Serialize(msg));
        this->socket->Write(SlokedSpan(reinterpret_cast<const uint8_t *>(raw.data()), raw.size()));
        this->socket->Write('\0');
    }

    void KgrNetInterface::ActionInvoke(const KgrValue &msg) {
        int64_t id = msg.AsDictionary()["id"].AsInt();
        const auto &method = msg.AsDictionary()["method"].AsString();
        const auto &params = msg.AsDictionary()["params"];
        Responder responder(*this, id);
        try {
            if (this->methods.count(method) != 0) {
                this->methods.at(method)(method, params, responder);
            } else {
                this->InvokeMethod(method, params, responder);
            }
        } catch (const SlokedError &err) {
            if (this->socket->Valid()) {
                responder.Error(err.what());
            } else {
                throw;
            }
        }
    }

    void KgrNetInterface::ActionResponse(const KgrValue &msg) {
        int64_t id = msg.AsDictionary()["id"].AsInt();
        std::unique_lock<std::mutex> lock(this->response_mtx);
        if (this->responses.count(id) != 0) {
            if (msg.AsDictionary().Has("result")) {
                this->responses.at(id).push(Response(msg.AsDictionary()["result"], true));
                this->response_cv.notify_all();
            } else if (msg.AsDictionary().Has("error")) {
                this->responses.at(id).push(Response(msg.AsDictionary()["error"], false));
                this->response_cv.notify_all();
            }
        }
    }

    void KgrNetInterface::ActionClose(const KgrValue &msg) {
        this->socket->Close();
        this->buffer.clear();
        this->incoming = {};
        std::unique_lock<std::mutex> lock(this->response_mtx);
        this->responses.clear();
        this->response_cv.notify_all();
    }
}