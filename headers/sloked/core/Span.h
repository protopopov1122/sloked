
/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_CORE_SPAN_H_
#define SLOKED_CORE_SPAN_H_

#include "sloked/core/Error.h"
#include <cinttypes>

namespace sloked {

    template <typename T>
    class SlokedSpan {
     public:
        SlokedSpan()
            : data_ptr(nullptr), sz(0) {}

        SlokedSpan(T *data, std::size_t size)
            : data_ptr(data), sz(size) {}

        SlokedSpan(const SlokedSpan<T> &) = default;
        SlokedSpan(SlokedSpan<T> &&span)
            : data_ptr(span), sz(span.sz) {
            span.data_ptr = nullptr;
            span.sz = 0;
        }

        SlokedSpan<T> &operator=(const SlokedSpan<T> &) = default;
        SlokedSpan<T> &operator=(SlokedSpan<T> &&span) {
            this->data_ptr = span.data_ptr;
            this->sz = span.sz;
            span.data_ptr = nullptr;
            span.sz = 0;
        }

        T *Data() const {
            return this->data_ptr;
        }

        std::size_t Size() const {
            return this->sz;
        }

        bool Empty() const {
            return this->data_ptr == nullptr ||
                this->sz == 0;
        }

        T &operator[](std::size_t idx) const {
            if (idx > this->sz || this->data_ptr == nullptr) {
                throw SlokedError("SlokedSpan: Access out of bounds");
            } else {
                return this->data_ptr[idx];
            }
        }

        T &Front() const {
            if (this->Empty()) {
                throw SlokedError("SlokedSpan: Access out of bounds");
            } else {
                return this->data_ptr[0];
            }
        }

        T &Back() const {
            if (this->Empty()) {
                throw SlokedError("SlokedSpan: Access out of bounds");
            } else {
                return this->data_ptr[this->sz - 1];
            }
        }

        SlokedSpan<T> Subspan(std::size_t start, std::size_t count) {
            if (count == 0) {
                return SlokedSpan<T>(nullptr, 0);
            }
            if (this->data_ptr == nullptr ||
                start + count >= this->sz) {
                throw SlokedError("SlokedSpan: Subspan out of bounds");
            } else {
                return SlokedSpan<T>(this->data_ptr + start, count);
            }
        }

     private:
        T *data_ptr;
        std::size_t sz;
    };

    template <typename T>
    using SlokedConstSpan = SlokedSpan<const T>;
}

#endif