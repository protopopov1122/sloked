#ifndef SLOKED_CORE_RANGEMAP_H_
#define SLOKED_CORE_RANGEMAP_H_

#include "sloked/Base.h"
#include "sloked/core/Error.h"
#include <map>
#include <limits>
#include <optional>

namespace sloked {

    template<typename Key, typename Value>
    class RangeMap {
     public:
        RangeMap(const Key &minKey = std::numeric_limits<Key>::lowest())
            : minKey(minKey) {
            this->ranges[minKey] = std::optional<Value>{};
        }

        RangeMap(const RangeMap &) = default;
        RangeMap(RangeMap &&) = default;
        virtual ~RangeMap() = default;

        RangeMap &operator=(const RangeMap &) = default;
        RangeMap &operator=(RangeMap &&) = default;

        const Value &At(const Key &key) const {
            const std::optional<Value> &range = std::prev(this->ranges.upper_bound(key))->second;
            if (range.has_value()) {
                return range.value();
            } else {
                throw SlokedError("Range not found");
            }
        }

        bool Has(const Key &key) const {
            return std::prev(this->ranges.upper_bound(key))->second.has_value();
        }

        void Insert(const Key &begin, const Key &end, const Value &value) {
            if (begin < this->minKey) {
                throw SlokedError("Invalid range");
            }
            if (!(begin < end)) {
                throw SlokedError("Range with non-positive length");
            }

            RangeIterator rangeBegin = this->ranges.lower_bound(begin);
            RangeIterator rangeEnd = this->ranges.upper_bound(end);
            RangeIterator rangesEnd = this->ranges.end();

            bool insertAtBegin = false;
            RangeIterator outerRangeBegin = this->ranges.begin();
            if (rangeBegin != this->ranges.begin()) {
                outerRangeBegin = std::prev(rangeBegin);
            } else {
                insertAtBegin = true;
            }
            std::optional<Value> outerRangeTail = std::prev(rangeEnd)->second;
            
            this->ranges.erase(rangeBegin, rangeEnd);
            
            if (!insertAtBegin && outerRangeBegin == rangesEnd) {
                this->ranges.insert(outerRangeBegin, std::make_pair(begin, value));
            } else if (!insertAtBegin && !(outerRangeBegin->second == value)) {
                this->ranges.insert(std::next(outerRangeBegin), std::make_pair(begin, value));
            } else if (insertAtBegin) {
                this->ranges.insert_or_assign(this->ranges.begin(), begin, value);
            }

            if ((rangeEnd == rangesEnd ||
                !(std::prev(rangeEnd)->second == outerRangeTail)) &&
                !(outerRangeTail == value)) {
                this->ranges.insert(rangeEnd, std::make_pair(end, outerRangeTail));
            }
        }

     private:
        using RangeIterator = typename std::map<Key, std::optional<Value>>::const_iterator;

        const Key minKey;
        std::map<Key, std::optional<Value>> ranges;
    };
}


#endif