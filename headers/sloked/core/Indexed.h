#ifndef SLOKED_CORE_INDEXED_H_
#define SLOKED_CORE_INDEXED_H_

#include "sloked/Base.h"
#include <cinttypes>

namespace sloked {

    template <typename T, typename I = std::size_t>
    struct SlokedIndexed {
        SlokedIndexed(I index, T &&value)
            : index(index), value(std::forward<T>(value)){}

        operator T &() {
            return this->value;
        }

        I index;
        T value;
    };
}

#endif