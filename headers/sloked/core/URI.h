#ifndef SLOKED_CORE_URI_H_
#define SLOKED_CORE_URI_H_

#include "sloked/Base.h"
#include <string>

namespace sloked {

    class SlokedUri {
     public:
        static std::string encodeComponent(std::string_view);
    };
}

#endif