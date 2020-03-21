/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

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

#ifndef SLOKED_CORE_LRU_H_
#define SLOKED_CORE_LRU_H_

#include "sloked/core/Error.h"
#include <map>
#include <queue>

namespace sloked {

    template <typename Key, typename Value, typename Less = std::less<Key>>
    class SlokedLRUCache {
     public:
        SlokedLRUCache(std::size_t capacity)
            : capacity{capacity} {}

        void Insert(const Key &key, const Value &value) {
            while (this->cache.size() > this->capacity) {
                cache.erase(this->lru.front());
                this->lru.pop();
            }
            this->cache.insert_or_assign(key, value);
            this->lru.push(key);
        }

        void Emplace(const Key &key, Value &&value) {
            while (this->cache.size() > this->capacity) {
                cache.erase(this->lru.front());
                this->lru.pop();
            }
            this->cache.emplace(key, std::forward<Value>(value));
            this->lru.push(key);
        }

        bool Has(const Key &key) const {
            return this->cache.count(key) != 0;
        }

        const Value &At(const Key &key) const {
            if (this->Has(key)) {
                return this->cache.at(key);
            } else {
                throw SlokedError("LRUCache: Entry not found");
            }
        }

        Value &At(const Key &key) {
            if (this->Has(key)) {
                return this->cache.at(key);
            } else {
                throw SlokedError("LRUCache: Entry not found");
            }
        }

     private:
        std::size_t capacity;
        std::map<Key, Value, Less> cache;
        std::queue<Key> lru; 
    };
}

#endif