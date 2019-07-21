#ifndef SLOKED_CORE_ERROR_H_
#define SLOKED_CORE_ERROR_H_

#include "sloked/Base.h"
#include <string>
#include <exception>

namespace sloked {

    class SlokedError : public std::exception {
     public:
        SlokedError(const std::string_view);
        const char *what() const noexcept override;
     private:
        std::string message;
    };
}

#endif