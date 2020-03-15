#ifndef SLOKED_SCREEN_SIZE_H_
#define SLOKED_SCREEN_SIZE_H_

#include "sloked/core/Position.h"
#include <functional>

namespace sloked {

    class SlokedScreenSize {
     public:
        using Listener = std::function<void(const TextPosition &)>;
        virtual ~SlokedScreenSize() = default;
        virtual TextPosition GetScreenSize() const = 0;
        virtual std::function<void()> Listen(Listener) = 0;
    };
}

#endif