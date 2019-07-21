#include "sloked/core/Error.h"

namespace sloked {

    SlokedError::SlokedError(const std::string_view msg)
        : message(msg) {

    }

    const char *SlokedError::what() const noexcept {
        return this->message.c_str();
    }
}