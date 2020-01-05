#ifndef SLOKED_CORE_HASH_H_
#define SLOKED_CORE_HASH_H_

#include "sloked/Base.h"
#include <cinttypes>
#include <array>
#include <algorithm>

namespace sloked {

    class SlokedCrc32 {
        using LookupTable_t = std::array<uint32_t, 256>;
     public:
        using Checksum = uint32_t;

        template <typename Iter>
        static Checksum Calculate(Iter begin, Iter end) {
            constexpr auto LookupTable = []() constexpr {
                LookupTable_t table{};
                constexpr Checksum reversed_polynomial{0xEDB88320uL};
                for (std::size_t n = 0; n < table.size(); n++) {
                    auto checksum = n;
                    for (auto i = 0; i < 8; i++) {
                        checksum = (checksum >> 1) ^ ((checksum & 0x1u) ? reversed_polynomial : 0);
                    }
                    table[n] = checksum;
                }
                return table;
            }();
            
            Checksum checksum{0xffffffffuL};
            for (auto it = begin; it != end; ++it) {
                const auto byte = *it;
                std::size_t idx = (checksum ^ byte) & 0xff;
                checksum = (checksum >> 8) ^ LookupTable[idx];
            }
            checksum ^= 0xffffffffuL;
            return checksum;
        }
    };
}

#endif