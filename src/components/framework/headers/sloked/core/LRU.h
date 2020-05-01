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

#ifndef SLOKED_CORE_LRU_H_
#define SLOKED_CORE_LRU_H_

#include <list>
#include <map>
#include <memory>

#include "sloked/core/Error.h"

namespace sloked {

    template <typename Key, typename Value, typename Less = std::less<Key>>
    class SlokedLRUCache {
        struct Entry {
            Key key;
            Value value;
            typename std::list<Entry *>::iterator iter;
        };

     public:
        SlokedLRUCache(std::size_t capacity) : capacity{capacity} {}

        void Insert(const Key &key, const Value &value) {
            this->DropOldest();
            auto entry = std::make_unique<Entry>();
            entry->key = key;
            entry->value = value;
            entry->iter = this->lru.insert(this->lru.end(), entry.get());
            this->cache.insert_or_assign(key, std::move(entry));
        }

        void Emplace(const Key &key, Value &&value) {
            this->DropOldest();
            auto entry = std::make_unique<Entry>();
            entry->key = key;
            entry->value = std::forward<Value>(value);
            entry->iter = this->lru.insert(this->lru.end(), entry.get());
            this->cache.insert_or_assign(key, std::move(entry));
        }

        bool Has(const Key &key) const {
            return this->cache.count(key) != 0;
        }

        const Value &At(const Key &key) const {
            if (this->Has(key)) {
                auto entry = this->cache.at(key).get();
                this->lru.erase(entry->iter);
                entry->iter = this->lru.insert(this->lru.end(), entry);
                return entry->value;
            } else {
                throw SlokedError("LRUCache: Entry not found");
            }
        }

        Value &At(const Key &key) {
            if (this->Has(key)) {
                auto entry = this->cache.at(key).get();
                this->lru.erase(entry->iter);
                entry->iter = this->lru.insert(this->lru.end(), entry);
                return entry->value;
            } else {
                throw SlokedError("LRUCache: Entry not found");
            }
        }

     private:
        void DropOldest() {
            while (this->cache.size() > this->capacity) {
                Entry *oldest = this->lru.front();
                this->lru.erase(oldest->iter);
                this->cache.erase(oldest->key);
            }
        }

        std::size_t capacity;
        std::map<Key, std::unique_ptr<Entry>, Less> cache;
        std::list<Entry *> lru;
    };
}  // namespace sloked

#endif