#ifndef SLOKED_CORE_PERMISSION_H_
#define SLOKED_CORE_PERMISSION_H_

#include "sloked/Base.h"

namespace sloked {

    template <typename T>
    class SlokedPermissionAuthority {
     public:
        virtual ~SlokedPermissionAuthority() = default;
        virtual bool HasPermission(T) const = 0;
    };
}

#endif