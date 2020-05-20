#ifndef SLOKED_NAMESPACE_WIN32_PATH_H_
#define SLOKED_NAMESPACE_WIN32_PATH_H_

#include "sloked/namespace/Path.h"

namespace sloked {

    SlokedPath Win32Path(std::string_view);
    std::string PathToWin32(const SlokedPath &);
}

#endif