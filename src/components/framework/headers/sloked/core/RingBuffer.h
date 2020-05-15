/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_CORE_RINGBUFFER_H_
#define SLOKED_CORE_RINGBUFFER_H_

#include <type_traits>

#include "sloked/core/Error.h"

namespace sloked {

    enum class SlokedRingBufferType { Static, Dynamic };

    template <typename T,
              SlokedRingBufferType BufferType = SlokedRingBufferType::Static>
    class SlokedRingBuffer {
        static constexpr std::size_t Padding = 1;
        using AlignedType = std::aligned_storage_t<sizeof(T), alignof(T)>;

     public:
        using ElementType = T;
        using IndexType = std::size_t;
        using DiffType = int64_t;

        template <typename Self>
        class AbstractIterator
            : public std::iterator<std::input_iterator_tag, T, IndexType,
                                   const T *, const T> {
         public:
            using Ring = SlokedRingBuffer<T, BufferType>;
            friend Ring;

            Self &operator++() {
                this->idx = ring.shift(this->idx, 1);
                return *static_cast<Self *>(this);
            }

            Self operator++(int) {
                Self iter{*static_cast<Self *>(this)};
                this->idx = ring.shift(this->idx, 1);
                return iter;
            }

            Self &operator--() {
                this->idx = ring.shift(this->idx, -1);
                return *static_cast<Self *>(this);
            }

            Self operator--(int) {
                Self iter{*static_cast<Self *>(this)};
                this->idx = ring.shift(this->idx, -1);
                return iter;
            }

            Self operator+(DiffType diff) const {
                Self iter{*static_cast<const Self *>(this)};
                iter += diff;
                return iter;
            }

            Self operator-(DiffType diff) const {
                Self iter{*static_cast<const Self *>(this)};
                iter -= diff;
                return iter;
            }

            DiffType operator-(const Self &other) {
                if (std::addressof(this->ring) == std::addressof(other.ring)) {
                    auto currentAbs =
                        ring.total_capacity - this->ring.available_distance(
                                                  this->ring.head, this->idx);
                    auto otherAbs =
                        ring.total_capacity - this->ring.available_distance(
                                                  this->ring.head, other.idx);
                    return static_cast<DiffType>(currentAbs) -
                           static_cast<DiffType>(otherAbs);
                } else {
                    throw SlokedError(
                        "RingBufferIterator: Can't calculate difference "
                        "between different ring iterators");
                }
            }

            Self &operator+=(DiffType diff) {
                this->idx = this->ring.shift(this->idx, diff);
                return *static_cast<Self *>(this);
            }

            Self &operator-=(DiffType diff) {
                this->idx = this->ring.shift(this->idx, -diff);
                return *static_cast<Self *>(this);
            }

            auto operator[](IndexType idx) {
                return *(static_cast<const Self *>(this)->Self::operator+(idx));
            }

            bool operator==(const Self &other) const {
                return std::addressof(this->ring) ==
                           std::addressof(other.ring) &&
                       this->idx == other.idx;
            }

            bool operator!=(const Self &other) const {
                return !static_cast<const Self *>(this)->Self::operator==(
                    other);
            }

            bool operator<(const Self &other) const {
                if (std::addressof(this->ring) == std::addressof(other.ring)) {
                    auto currentAbs =
                        ring.total_capacity - this->ring.available_distance(
                                                  this->ring.head, this->idx);
                    auto otherAbs =
                        ring.total_capacity - this->ring.available_distance(
                                                  this->ring.head, other.idx);
                    return currentAbs < otherAbs;
                } else {
                    throw SlokedError(
                        "RingBufferIterator: Can't calculate difference "
                        "between different ring iterators");
                }
            }

            bool operator<=(const Self &other) const {
                return static_cast<const Self *>(this)->Self::operator==(
                           other) ||
                       static_cast<const Self *>(this)->Self::operator<(other);
            }

            bool operator>(const Self &other) const {
                return !static_cast<const Self *>(this)->Self::operator<=(
                    other);
            }

            bool operator>=(const Self &other) const {
                return static_cast<const Self *>(this)->Self::operator==(
                           other) ||
                       static_cast<const Self *>(this)->Self::operator>(other);
            }

            const T &operator*() const {
                return *reinterpret_cast<T *>(&ring.buffer[idx]);
            }

         protected:
            explicit AbstractIterator(const Ring &ring, IndexType idx)
                : ring(ring), idx(idx) {}

            const Ring &ring;
            IndexType idx;
        };

        template <typename>
        friend class AbstractIterator;

        class ConstIterator : public AbstractIterator<ConstIterator> {
         public:
            using AbstractIterator<ConstIterator>::AbstractIterator;
            using Ring = typename AbstractIterator<ConstIterator>::Ring;
            friend Ring;
        };

        SlokedRingBuffer(IndexType user_capacity)
            : head{0}, tail{0}, total_capacity{user_capacity + Padding} {
            this->buffer = this->allocate(this->total_capacity);
        }

        SlokedRingBuffer(const SlokedRingBuffer<T, BufferType> &) = delete;

        SlokedRingBuffer(SlokedRingBuffer<T, BufferType> &&ring) {
            this->buffer = ring.buffer;
            this->total_capacity = ring.total_capacity;
            this->head = ring.head;
            this->tail = ring.tail;
            ring.reset();
        }

        ~SlokedRingBuffer() {
            auto count = this->size();
            while (count--) {
                if constexpr (std::is_destructible_v<T>) {
                    reinterpret_cast<T *>(&this->buffer[this->head])->T::~T();
                }
                this->head = this->shift_forward(this->head, 1);
            }
            this->deallocate(this->buffer);
        }

        SlokedRingBuffer &operator=(const SlokedRingBuffer<T, BufferType> &) =
            delete;

        SlokedRingBuffer &operator=(
            const SlokedRingBuffer<T, BufferType> &&ring) {
            this->pop_front(this->size());
            this->deallocate(this->buffer);
            this->buffer = ring.buffer;
            this->total_capacity = ring.total_capacity;
            this->head = ring.head;
            this->tail = ring.tail;
            ring.reset();
        }

        constexpr SlokedRingBufferType GetType() const {
            return BufferType;
        }

        bool empty() const {
            return this->head == this->tail;
        }

        IndexType available() const {
            return this->available_distance(this->head, this->tail) - Padding;
        }

        IndexType size() const {
            return this->total_capacity -
                   this->available_distance(this->head, this->tail);
        }

        IndexType capacity() const {
            return this->total_capacity - Padding;
        }

        const T &front() const {
            if (this->empty()) {
                throw SlokedError("RingBuffer: Empty buffer");
            }
            return *reinterpret_cast<T *>(&this->buffer[this->head]);
        }

        T &front() {
            if (this->empty()) {
                throw SlokedError("RingBuffer: Empty buffer");
            }
            return *reinterpret_cast<T *>(&this->buffer[this->head]);
        }

        const T &back() const {
            if (this->empty()) {
                throw SlokedError("RingBuffer: Empty buffer");
            }
            return *reinterpret_cast<T *>(
                &this->buffer[this->shift_backward(this->tail, 1)]);
        }

        T &back() {
            if (this->empty()) {
                throw SlokedError("RingBuffer: Empty buffer");
            }
            return *reinterpret_cast<T *>(
                &this->buffer[this->shift_backward(this->tail, 1)]);
        }

        const T &at(IndexType idx) const {
            if (idx < this->size()) {
                return *reinterpret_cast<T *>(
                    &this->buffer[this->shift_forward(this->head, idx)]);
            } else {
                throw SlokedError("RingBuffer: Index out of bounds");
            }
        }

        T &at(IndexType idx) {
            if (idx < this->size()) {
                return *reinterpret_cast<T *>(
                    &this->buffer[this->shift_forward(this->head, idx)]);
            } else {
                throw SlokedError("RingBuffer: Index out of bounds");
            }
        }

        const T &operator[](IndexType idx) const {
            return this->at(idx);
        }

        T &operator[](IndexType idx) {
            return this->at(idx);
        }

        ConstIterator begin() const {
            return ConstIterator{*this, this->head};
        }

        ConstIterator end() const {
            return ConstIterator{*this, this->tail};
        }

        void push_back(const T &element) {
            this->ensure_available(1);
            this->construct(this->buffer + this->tail, element);
            this->tail = this->shift_forward(this->tail, 1);
        }

        void emplace_back(T &&element) {
            this->ensure_available(1);
            this->construct(this->buffer + this->tail,
                            std::forward<T>(element));
            this->tail = this->shift_forward(this->tail, 1);
        }

        template <typename I>
        void insert(I begin, I end) {
            for (auto it = begin; it != end; ++it) {
                this->push_back(*it);
            }
        }

        void pop_front(IndexType count = 1) {
            if (count > this->size()) {
                throw SlokedError("RingBuffer: Not enough elements to pop");
            }
            while (count--) {
                if constexpr (std::is_destructible_v<T>) {
                    reinterpret_cast<T *>(&this->buffer[this->head])->T::~T();
                }
                this->head = this->shift_forward(this->head, 1);
            }
        }

        void clear() {
            this->pop_front(this->size());
        }

        void reset() {
            this->pop_front(this->size());
            if constexpr (BufferType == SlokedRingBufferType::Dynamic) {
                this->deallocate(this->buffer);
                this->total_capacity = Padding;
                this->buffer = nullptr;
                this->head = 0;
                this->tail = 0;
            }
        }

        template <typename I>
        I collect(IndexType count, I inserter) {
            if (count > this->size()) {
                throw SlokedError(
                    "RingBuffer: Can't collect more than buffer size");
            }
            while (count--) {
                inserter++ = this->front();
                this->pop_front();
            }
            return inserter;
        }

        template <typename I>
        I collect(I inserter) {
            return this->collect(this->size(), std::move(inserter));
        }

     private:
        void construct(AlignedType *dest, T &&src) const {
            if constexpr (std::is_move_constructible_v<T>) {
                new (dest) T(std::forward<T>(src));
            } else {
                new (dest) T(src);
            }
        }

        void construct(AlignedType *dest, const T &src) const {
            new (dest) T(src);
        }

        AlignedType *allocate(std::size_t sz) const {
            return reinterpret_cast<AlignedType *>(operator new(
                sizeof(AlignedType) * sz));
        }

        void deallocate(AlignedType *ptr) const {
            operator delete(reinterpret_cast<void *>(ptr));
        }

        void reallocateBuffer(std::size_t nextCapacity) {
            auto nextBuffer = this->allocate(nextCapacity);
            if (this->head <= this->tail) {
                IndexType nextIdx = 0;
                for (auto idx = this->head; idx < this->tail;
                     ++idx, ++nextIdx) {
                    auto &prev = *reinterpret_cast<T *>(&this->buffer[idx]);
                    this->construct(nextBuffer + nextIdx, std::move(prev));
                    reinterpret_cast<T *>(std::addressof(prev))->T::~T();
                }
                this->deallocate(this->buffer);
                this->buffer = nextBuffer;
                this->total_capacity = nextCapacity;
                this->head = 0;
                this->tail = nextIdx;
            } else {
                IndexType nextIdx = 0;
                for (auto idx = this->head; idx < this->total_capacity;
                     ++idx, ++nextIdx) {
                    auto &prev = *reinterpret_cast<T *>(&this->buffer[idx]);
                    this->construct(nextBuffer + nextIdx, std::move(prev));
                    reinterpret_cast<T *>(std::addressof(prev))->T::~T();
                }
                for (IndexType idx = 0; idx < this->tail; ++idx, ++nextIdx) {
                    auto &prev = *reinterpret_cast<T *>(&this->buffer[idx]);
                    this->construct(nextBuffer + nextIdx, std::move(prev));
                    reinterpret_cast<T *>(std::addressof(prev))->T::~T();
                }
                this->deallocate(this->buffer);
                this->buffer = nextBuffer;
                this->total_capacity = nextCapacity;
                this->head = 0;
                this->tail = nextIdx;
            }
        }

        void ensure_available(IndexType sz) {
            if constexpr (BufferType == SlokedRingBufferType::Static) {
                if (sz > this->available()) {
                    throw SlokedError(
                        "RingBuffer: Can't allocate requested amount");
                }
            } else if (sz > this->available()) {
                auto nextCapacity = this->total_capacity * 3 / 2 + Padding;
                this->reallocateBuffer(nextCapacity);
            }
        }

        IndexType available_distance(IndexType first, IndexType second) const {
            return first <= second ? this->total_capacity - (second - first)
                                   : first - second;
        }

        IndexType shift(IndexType base, DiffType index) const {
            if (index > 0) {
                return this->shift_forward(base, static_cast<IndexType>(index));
            } else if (index < 0) {
                return this->shift_backward(
                    base, static_cast<IndexType>(std::abs(index)));
            } else {
                return base;
            }
        }

        IndexType shift_forward(IndexType base, IndexType index) const {
            index %= this->total_capacity;
            return (base + index) % this->total_capacity;
        }

        IndexType shift_backward(IndexType base, IndexType index) const {
            index %= this->total_capacity;
            if (base >= index) {
                return base - index;
            } else {
                return this->total_capacity - (index - base);
            }
        }

        AlignedType *buffer;
        IndexType head;
        IndexType tail;
        IndexType total_capacity;
    };
}  // namespace sloked

#endif