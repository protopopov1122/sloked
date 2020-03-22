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

#ifndef SLOKED_CORE_ORDEREDCACHE_H_
#define SLOKED_CORE_ORDEREDCACHE_H_

#include "sloked/core/Error.h"
#include <map>
#include <functional>
#include <type_traits>

namespace sloked {

    template <typename T, typename E = void>
    struct SlokedOrderedCacheKeyTraits;

    template <typename T>
    struct SlokedOrderedCacheKeyTraits<T, std::enable_if_t<std::is_integral_v<T>>> {
        constexpr T Next(const T &key) const {
            return key + 1;
        }

        constexpr bool Less(const T &first, const T &second) const {
            return first < second;
        }

        constexpr std::size_t Distance(const T &first, const T &second) const {
            return std::max(first, second) - std::min(first, second);
        }
    };

    template <typename Key, typename Value, typename KeyTraits = SlokedOrderedCacheKeyTraits<Key>>
    class SlokedOrderedCache {
        struct Comparator {
            Comparator(const KeyTraits &traits)
                : traits(traits) {}

            bool operator()(const Key &first, const Key &second) const {
                return traits.Less(first, second);
            }

            const KeyTraits &traits;
        };

     public:
        using Supplier = std::function<std::vector<Value>(const Key &, const Key &)>;
        using Iterator = typename std::map<Key, Value, Comparator>::const_iterator;
        using Range = std::pair<Iterator, Iterator>;

        SlokedOrderedCache(Supplier supplier, KeyTraits traits = {})
            : supplier(std::move(supplier)), traits(std::move(traits)), comparator{this->traits}, cache(this->comparator) {}

        Range Fetch(const Key &begin, const Key &end) {
            if (!this->traits.Less(begin, end) && this->traits.Distance(begin, end) > 0) {
                throw SlokedError("OrderedCache: Reversed order of requested range");
            }
            bool countingRange{false};
            std::pair<Key, Key> fetchRange;
            auto rangeIt = this->cache.end();
            for (auto key = begin; !traits.Less(end, key); key = traits.Next(key)) {
                rangeIt = this->cache.find(key);
                if (rangeIt == this->cache.end()) {
                    if (countingRange) {
                        fetchRange.second = key;
                    } else {
                        countingRange = true;
                        fetchRange = {key, key};
                    }
                } else if (countingRange) {
                    countingRange = false;
                    auto values = this->supplier(fetchRange.first, fetchRange.second);
                    auto distance = this->traits.Distance(fetchRange.first, fetchRange.second);
                    if (values.size() != distance + 1) {
                        throw SlokedError("OrderedCache: Supplied value range does not correspond to requested keys");
                    }
                    auto it = values.begin();
                    for (auto current = fetchRange.first; !this->traits.Less(fetchRange.second, current); current = this->traits.Next(current), ++it) {
                        this->cache.emplace(current, std::move(*it));
                    }
                }
            }
            if (countingRange) {
                auto values = this->supplier(fetchRange.first, fetchRange.second);
                if (values.size() != this->traits.Distance(fetchRange.first, fetchRange.second) + 1) {
                    throw SlokedError("OrderedCache: Supplied value range does not correspond to requested keys");
                }
                auto it = values.begin();
                for (auto current = fetchRange.first; !this->traits.Less(fetchRange.second, current); current = this->traits.Next(current), ++it) {
                    rangeIt = this->cache.emplace(current, std::move(*it)).first;
                }
            }
            return {this->cache.find(begin), std::next(rangeIt)};
        }

        std::vector<std::pair<Key, Iterator>> FetchUpdated(const Key &begin, const Key &end) {
            if (!this->traits.Less(begin, end) && this->traits.Distance(begin, end) > 0) {
                throw SlokedError("OrderedCache: Reversed order of requested range");
            }
            std::vector<std::pair<Key, Iterator>> result;
            bool countingRange{false};
            std::pair<Key, Key> fetchRange;
            auto rangeIt = this->cache.end();
            for (auto key = begin; !traits.Less(end, key); key = traits.Next(key)) {
                rangeIt = this->cache.find(key);
                if (rangeIt == this->cache.end()) {
                    if (countingRange) {
                        fetchRange.second = key;
                    } else {
                        countingRange = true;
                        fetchRange = {key, key};
                    }
                } else if (countingRange) {
                    countingRange = false;
                    auto values = this->supplier(fetchRange.first, fetchRange.second);
                    auto distance = this->traits.Distance(fetchRange.first, fetchRange.second);
                    if (values.size() != distance + 1) {
                        throw SlokedError("OrderedCache: Supplied value range does not correspond to requested keys");
                    }
                    auto it = values.begin();
                    for (auto current = fetchRange.first; !this->traits.Less(fetchRange.second, current); current = this->traits.Next(current), ++it) {
                        result.emplace_back(std::make_pair(current, this->cache.emplace(current, std::move(*it)).first));
                    }
                }
            }
            if (countingRange) {
                auto values = this->supplier(fetchRange.first, fetchRange.second);
                if (values.size() != this->traits.Distance(fetchRange.first, fetchRange.second) + 1) {
                    throw SlokedError("OrderedCache: Supplied value range does not correspond to requested keys");
                }
                auto it = values.begin();
                for (auto current = fetchRange.first; !this->traits.Less(fetchRange.second, current); current = this->traits.Next(current), ++it) {
                    result.emplace_back(std::make_pair(current, this->cache.emplace(current, std::move(*it)).first));
                }
            }
            return result;
        }

        void Drop(const Key &begin, const Key &end) {
            this->cache.erase(this->cache.lower_bound(begin), this->cache.upper_bound(end));
        }

        void Clear() {
            this->cache.clear();
        }

        template <typename I>
        void Insert(const I &begin, const I &end) {
            for (auto it = begin; it != end; ++it) {
                cache.insert_or_assign(it->first, it->second);
            }
        }

    
     private:
        Supplier supplier;
        KeyTraits traits;
        Comparator comparator;
        std::map<Key, Value, Comparator> cache;
    };
}

#endif