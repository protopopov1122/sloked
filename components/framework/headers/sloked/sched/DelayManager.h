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

#ifndef SLOKED_SCHED_DELAYMANAGER_H_
#define SLOKED_SCHED_DELAYMANAGER_H_

#include <map>
#include <mutex>

#include "sloked/sched/Scheduler.h"

namespace sloked {

    template <typename Key>
    class SlokedDelayManager {
     public:
        using Callback = std::function<void()>;

        SlokedDelayManager(SlokedScheduler &sched) : sched(sched) {}

        void Schedule(const Key &key, SlokedScheduler::TimeDiff tdiff,
                      Callback callback) {
            std::shared_ptr<SlokedScheduler::TimerTask> prevDelay;
            std::unique_lock lock(this->mtx);
            if (this->delays.count(key)) {
                prevDelay = this->delays.at(key);
                prevDelay->Cancel();
            }
            this->delays.insert_or_assign(
                key, this->sched.Sleep(tdiff, std::move(callback)));
            lock.unlock();
            if (prevDelay) {
                prevDelay->Wait();
            }
        }

        void Cancel(const Key &key) {
            std::shared_ptr<SlokedScheduler::TimerTask> prevDelay;
            std::unique_lock lock(this->mtx);
            if (this->delays.count(key)) {
                prevDelay = this->delays.at(key);
                prevDelay->Cancel();
            }
            lock.unlock();
            if (prevDelay) {
                prevDelay->Wait();
            }
        }

        void Clear() {
            std::unique_lock lock(this->mtx);
            while (!this->delays.empty()) {
                auto delay = *this->delays.begin();
                this->delays.erase(this->delays.begin());
                lock.unlock();
                delay.second->Cancel();
                delay.second->Wait();
                lock.lock();
            }
            this->delays.clear();
        }

     private:
        SlokedScheduler &sched;
        std::mutex mtx;
        std::map<Key, std::shared_ptr<SlokedScheduler::TimerTask>> delays;
    };
}  // namespace sloked

#endif