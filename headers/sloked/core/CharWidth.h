#ifndef SLOKED_CORE_CHARWIDTH_H_
#define SLOKED_CORE_CHARWIDTH_H_

#include "sloked/Base.h"
#include "sloked/core/Encoding.h"
#include <cinttypes>

namespace sloked {

    class SlokedCharWidth {
     public:
        SlokedCharWidth();
        std::size_t GetCharWidth(char32_t) const;
        std::pair<std::size_t, std::size_t> GetRealPosition(const std::string &, std::size_t, const Encoding &) const;
        std::string GetTab() const;
    
        void SetTabWidth(std::size_t);
     private:
        std::size_t tab_width;
    };
}

#endif