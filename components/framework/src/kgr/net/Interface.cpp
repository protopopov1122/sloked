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

#include "sloked/kgr/net/Interface.h"

#include <chrono>

#include "sloked/core/Error.h"
#include "sloked/core/Locale.h"
#include "sloked/kgr/Serialize.h"
#include "sloked/kgr/net/Config.h"

using namespace std::chrono_literals;

namespace sloked {

    using DefaultSerializer = KgrBinarySerializer;

    KgrNetInterface::Responder::Responder(KgrNetInterface &net, int64_t id)
        : net(net), id(id) {}

    void KgrNetInterface::Responder::Result(const KgrValue &value) {
        this->net.Write(KgrDictionary{
            {"action", "response"}, {"id", this->id}, {"result", value}});
    }

    void KgrNetInterface::Responder::Error(const KgrValue &value) {
        this->net.Write(KgrDictionary{
            {"action", "response"}, {"id", this->id}, {"error", value}});
    }

    KgrNetInterface::KgrNetInterface(std::unique_ptr<SlokedSocket> socket,
                                     SlokedScheduler &sched)
        : socket(std::move(socket)), buffer{0}, inactivityTimeouts(sched) {}

    bool KgrNetInterface::Wait(
        std::chrono::system_clock::duration timeout) const {
        return !this->buffer.empty() || this->socket->Wait(timeout);
    }

    std::size_t KgrNetInterface::Available() const {
        return this->socket->Valid()
                   ? this->socket->Available() + this->buffer.size()
                   : 0;
    }

    bool KgrNetInterface::Valid() const {
        return this->socket->Valid();
    }

    void KgrNetInterface::Receive() {
        if (this->socket->Available() > 0) {
            DefaultSerializer serializer;
            auto data = this->socket->Read(socket->Available());
            this->buffer.insert(data.begin(), data.end());
            while (this->buffer.size() >= 4) {
                std::size_t length =
                    this->buffer.at(0) | (this->buffer.at(1) << 8) |
                    (this->buffer.at(2) << 16) | (this->buffer.at(3) << 24);
                if (this->buffer.size() < length + 4) {
                    break;
                }
                KgrSerializer::Blob message{this->buffer.begin() + 4,
                                            this->buffer.begin() + length + 4};
                this->buffer.pop_front(length + 4);
                try {
                    this->incoming.push(serializer.Deserialize(message));
                } catch (const SlokedError &err) {
                    throw SlokedError("KgrNetInterface: Malformed request");
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
                } else {
                    throw SlokedError("KgrNetInterface: Malformed request");
                }
            } catch (const SlokedError &err) {
                throw SlokedError("KgrNetInterface: Malformed request");
            }
        }
    }

    std::shared_ptr<SlokedNetResponseBroker::Channel> KgrNetInterface::Invoke(
        const std::string &method, const KgrValue &params) {
        auto res = this->responseBroker.OpenChannel();
        this->Write(KgrDictionary{{"action", "invoke"},
                                  {"id", res->GetID()},
                                  {"method", method},
                                  {"params", params}});
        this->inactivityTimeouts.Schedule(
            res->GetID(), KgrNetConfig::ResponseTimeout,
            [this, id = res->GetID()] {
                this->responseBroker.DropChannel(id);
            });
        return res;
    }

    void KgrNetInterface::Close() {
        this->Write(KgrDictionary{{"action", "close"}});
        this->inactivityTimeouts.Clear();
        this->socket->Close();
        this->buffer.clear();
        this->incoming = {};
        this->responseBroker.Close();
    }

    void KgrNetInterface::BindMethod(
        const std::string &method,
        std::function<void(const std::string &, const KgrValue &, Responder &)>
            handler) {
        this->methods[method] = handler;
    }

    std::unique_ptr<SlokedIOAwaitable> KgrNetInterface::Awaitable() const {
        return this->socket->Awaitable();
    }

    SlokedSocketEncryption *KgrNetInterface::GetEncryption() {
        return this->socket->GetEncryption();
    }

    void KgrNetInterface::InvokeMethod(const std::string &method,
                                       const KgrValue &params, Responder &rsp) {
        rsp.Error("Unknown method: " + method);
    }

    void KgrNetInterface::Write(const KgrValue &msg) {
        DefaultSerializer serializer;
        auto raw = serializer.Serialize(msg);
        if (raw.size() > std::numeric_limits<uint32_t>::max()) {
            throw SlokedError(
                "KgrNetInterface: Message length exceeds maximum");
        }
        std::array<uint8_t, 4> length{
            static_cast<uint8_t>(raw.size() & 0xff),
            static_cast<uint8_t>((raw.size() >> 8) & 0xff),
            static_cast<uint8_t>((raw.size() >> 16) & 0xff),
            static_cast<uint8_t>((raw.size() >> 24) & 0xff),
        };
        std::unique_lock lock(this->write_mtx);
        this->socket->Write(SlokedSpan(
            reinterpret_cast<const uint8_t *>(length.data()), length.size()));
        this->socket->Write(SlokedSpan(
            reinterpret_cast<const uint8_t *>(raw.data()), raw.size()));
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
        this->inactivityTimeouts.Schedule(
            id, KgrNetConfig::ResponseTimeout,
            [this, id] { this->responseBroker.DropChannel(id); });
        if (msg.AsDictionary().Has("result")) {
            responseBroker.Feed(id, SlokedNetResponseBroker::Response(
                                        msg.AsDictionary()["result"], true));
        } else if (msg.AsDictionary().Has("error")) {
            responseBroker.Feed(id, SlokedNetResponseBroker::Response(
                                        msg.AsDictionary()["error"], false));
        }
    }

    void KgrNetInterface::ActionClose(const KgrValue &msg) {
        this->inactivityTimeouts.Clear();
        this->socket->Close();
        this->buffer.clear();
        this->incoming = {};
        this->responseBroker.Close();
    }
}  // namespace sloked