#ifndef SLOKED_CORE_DYNAMICBITSET_H_
#define SLOKED_CORE_DYNAMICBITSET_H_

#include "sloked/Base.h"
#include <vector>
#include <cinttypes>
#include <climits>

namespace sloked {

    class SlokedDynamicBitset {
        using Integer = uint64_t;
     public:
        static constexpr std::size_t MinWidth = sizeof(Integer) * CHAR_BIT;
        SlokedDynamicBitset();
        std::size_t Count() const;
        bool Get(std::size_t) const;
        void Set(std::size_t, bool);
        std::size_t Allocate();

     private:
        std::vector<Integer> bits;
    };
}

#endif