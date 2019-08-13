#include "sloked/core/CharWidth.h"

namespace sloked {

    SlokedCharWidth::SlokedCharWidth()
        : tab_width(4) {}

    std::size_t SlokedCharWidth::GetCharWidth(char32_t chr) const {
        switch (chr) {
            case '\t':
                return this->tab_width;
            
            default:
                return 1;
        }
    }
    
    std::size_t SlokedCharWidth::GetRealPosition(const std::string &str, std::size_t idx, const Encoding &encoding) const {
        std::size_t pos = 0;
        if (idx == 0) {
            return pos;
        }
        idx--;
        encoding.IterateCodepoints(str, [&](auto start, auto length, auto value) {
            pos += GetCharWidth(value);
            return idx--;
        });
        return pos;
    }

    std::string SlokedCharWidth::GetTab() const {
        return std::string(this->tab_width, ' ');
    }

    void SlokedCharWidth::SetTabWidth(std::size_t width) {
        this->tab_width = width;
    }
}
