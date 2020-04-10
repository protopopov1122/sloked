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

#include "sloked/kgr/net/Response.h"

namespace sloked {

    SlokedNetResponseBroker::Response::Response(const KgrValue &content,
                                                bool result)
        : content{content}, result{result} {}

    bool SlokedNetResponseBroker::Response::HasResult() const {
        return this->result;
    }

    const KgrValue &SlokedNetResponseBroker::Response::GetResult() const {
        if (this->result) {
            return this->content;
        } else {
            throw SlokedError("KgrNetInterface: Result not available");
        }
    }

    const KgrValue &SlokedNetResponseBroker::Response::GetError() const {
        if (!this->result) {
            return this->content;
        } else {
            throw SlokedError("KgrNetInterface: Error not available");
        }
    }

    SlokedNetResponseBroker::SimplexChannel::SimplexChannel(
        SlokedNetResponseBroker &broker, int64_t id)
        : broker{broker}, id{id} {}

    SlokedNetResponseBroker::SimplexChannel::~SimplexChannel() {
        this->broker.DropChannel(this->id);
    }

    SlokedNetResponseBroker::SimplexChannel::Id
        SlokedNetResponseBroker::SimplexChannel::GetID() const {
        return this->id;
    }

    TaskResult<SlokedNetResponseBroker::Response>
        SlokedNetResponseBroker::SimplexChannel::Next() {
        std::unique_lock lock(this->mtx);
        TaskResultSupplier<Response> supplier;
        auto result = supplier.Result();
        if (this->pending.empty()) {
            auto self = this->broker.GetChannel(this->id);
            this->awaiting.emplace_back(
                std::make_pair(std::move(supplier), std::move(self)));
        } else {
            auto res = std::move(this->pending.front());
            this->pending.pop_front();
            lock.unlock();
            supplier.SetResult(std::move(res));
        }
        return result;
    }

    void SlokedNetResponseBroker::SimplexChannel::Push(Response rsp) {
        std::unique_lock lock(this->mtx);
        if (this->awaiting.empty()) {
            this->pending.emplace_back(std::move(rsp));
        } else {
            auto supplier = std::move(this->awaiting.front());
            this->awaiting.pop_front();
            lock.unlock();
            supplier.first.SetResult(std::move(rsp));
        }
    }

    void SlokedNetResponseBroker::SimplexChannel::Close() {
        std::unique_lock lock(this->mtx);
        this->pending.clear();
        for (auto &supplier : this->awaiting) {
            supplier.first.Cancel();
        }
        this->awaiting.clear();
    }

    std::shared_ptr<SlokedNetResponseBroker::Channel>
        SlokedNetResponseBroker::OpenChannel() {
        std::unique_lock lock(this->mtx);
        auto res = std::make_shared<SimplexChannel>(*this, this->nextId++);
        this->active.insert_or_assign(res->GetID(), res);
        return res;
    }

    std::shared_ptr<SlokedNetResponseBroker::Channel>
        SlokedNetResponseBroker::GetChannel(Channel::Id id) const {
        std::unique_lock lock(this->mtx);
        if (this->active.count(id)) {
            return this->active.at(id).lock();
        } else {
            return nullptr;
        }
    }

    void SlokedNetResponseBroker::DropChannel(Channel::Id id) {
        std::unique_lock lock(this->mtx);
        if (this->active.count(id)) {
            if (auto channel = this->active.at(id).lock()) {
                channel->Close();
            }
        }
        this->active.erase(id);
    }

    void SlokedNetResponseBroker::Feed(Channel::Id id, Response rsp) {
        std::unique_lock<std::mutex> lock(this->mtx);
        if (this->active.count(id) != 0) {
            auto channel = this->active.at(id).lock();
            if (channel == nullptr) {
                this->active.erase(id);
                return;
            }
            lock.unlock();
            channel->Push(std::move(rsp));
        }
    }

    void SlokedNetResponseBroker::Close() {
        std::unique_lock<std::mutex> lock(this->mtx);
        std::vector<std::shared_ptr<SimplexChannel>> channels;
        for (auto ch : this->active) {
            auto channel = ch.second.lock();
            if (channel) {
                channel->Close();
                channels.emplace_back(std::move(channel));
            }
        }
        this->active.clear();
        lock.unlock();
        channels.clear();
    }
}  // namespace sloked