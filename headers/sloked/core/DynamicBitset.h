#ifndef SLOKED_CORE_DYNAMICBITSET_H_
#define SLOKED_CORE_DYNAMICBITSET_H_

#include "sloked/Base.h"
#include <vector>
#include <cinttypes>

namespace sloked {

    class SlokedDynamicBitset {
     public:
        SlokedDynamicBitset();
        std::size_t Count() const;
        bool Get(std::size_t) const;
        void Set(std::size_t, bool);
        std::size_t Allocate();

     private:
        using Integer = uint64_t;
        std::vector<uint64_t> bits;
    };
}

#endif