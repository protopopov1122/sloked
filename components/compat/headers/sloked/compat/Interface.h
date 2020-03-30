#ifndef SLOKED_COMPAT_INTERFACE_H_
#define SLOKED_COMPAT_INTERFACE_H_

#include "sloked/editor/BaseInterface.h"

namespace sloked {

    class SlokedEditorCompat {
     public:
        static const SlokedBaseInterface &GetBaseInterface();
    };
}  // namespace sloked

#endif