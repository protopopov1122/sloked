#include "sloked/core/DynamicBitset.h"
#include "sloked/core/Error.h"
#include <limits>
#include <optional>

namespace sloked {

    template <typename T = uint64_t>
    static constexpr std::pair<std::size_t, std::size_t> CalcPosition(std::size_t idx) {
        const std::size_t offset = idx % (sizeof(T) * 8);
        const std::size_t position = idx / (sizeof(T) * 8);
        return {position, offset};
    }

    template <typename T = uint64_t>
    static constexpr std::size_t CalcBitOffset(std::size_t position, std::size_t offset) {
        return position * (sizeof(T) * 8) + offset;
    }

    template <typename T>
    static constexpr bool BitEnabled(T integer, std::size_t offset) {
        return ((integer >> offset) & 0b1) != 0;
    }

    template <typename T>
    static constexpr T EnableBit(T integer, std::size_t offset) {
        return integer | (1ul << offset);
    }

    template <typename T>
    static constexpr T DisableBit(T integer, std::size_t offset) {
        return integer & ~(1 << offset);
    }

    template <typename T>
    static std::optional<std::size_t> FindFree(T integer) {
        for (std::size_t i = 0; i < sizeof(T) * 8; i++) {
            if (!BitEnabled(integer, i)) {
                return i;
            }
        }
        return {};
    }

    SlokedDynamicBitset::SlokedDynamicBitset()
        : bits{} {}
    
    std::size_t SlokedDynamicBitset::Count() const {
        return this->bits.size() * sizeof(Integer) * 8;
    }

    bool SlokedDynamicBitset::Get(std::size_t idx) const {
        if (idx >= this->Count()) {
            throw SlokedError("DynamicBitset: Out of range " + std::to_string(idx));
        } else {
            auto [position, offset] = CalcPosition<Integer>(idx);
            auto integer = this->bits.at(position);
            return BitEnabled(integer, offset);
        }
    }

    void SlokedDynamicBitset::Set(std::size_t idx, bool value) {
        if (idx >= this->Count()) {
            this->bits.insert(this->bits.end(), (idx - this->Count()) / 8 + 1, 0);
        }
        auto [position, offset] = CalcPosition<Integer>(idx);
        auto integer = this->bits.at(position);
        if (value) {
            this->bits[position] = EnableBit(integer, offset);
        } else {
            this->bits[position] = DisableBit(integer, offset);
        }
    }

    std::size_t SlokedDynamicBitset::Allocate() {
        for (std::size_t position = 0; position < this->bits.size(); position++) {
            auto integer = this->bits.at(position);
            auto offset = FindFree(integer);
            if (offset.has_value()) {
                this->bits[position] = EnableBit(integer, offset.value());
                return CalcBitOffset<Integer>(position, offset.value());
            }
        }
        this->bits.push_back(0b1);
        return (this->bits.size() - 1) * sizeof(Integer) * 8;
    }
}