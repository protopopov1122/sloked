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

#ifndef SLOKED_CORE_RESOURCE_H_
#define SLOKED_CORE_RESOURCE_H_

#include <map>
#include <memory>
#include <optional>
#include <utility>

#include "sloked/core/Error.h"

namespace sloked {

    template <typename T>
    class SlokedRegistry {
     public:
        using Key = uint32_t;
        friend class Resource;
        class Resource {
         public:
            friend class SlokedRegistry<T>;

            Resource(SlokedRegistry<T> &registry)
                : key(SlokedRegistry<T>::None), registry(std::ref(registry)) {}

            Resource(const Resource &handle)
                : key(handle.key), registry(handle.registry) {
                this->registry.get().Use(this->key);
            }

            Resource(Resource &&handle)
                : key(handle.key), registry(handle.registry) {
                handle.key = SlokedRegistry<T>::None;
            }

            ~Resource() {
                this->registry.get().Unuse(this->key);
            }

            Resource &operator=(const Resource &handle) {
                this->registry.get().Unuse(this->key);
                this->key = handle.key;
                this->registry = handle.registry;
                this->registry.get().Use(this->key);
                return *this;
            }

            Resource &operator=(Resource &&handle) {
                this->registry.get().Unuse(this->key);
                this->key = handle.key;
                this->registry = handle.registry;
                handle.key = SlokedRegistry<T>::None;
                return *this;
            }

            Key GetKey() const {
                return this->key;
            }

            bool Exists() const {
                return this->key != SlokedRegistry<T>::None;
            }

            T &GetObject() const {
                if (this->key != SlokedRegistry<T>::None) {
                    return *this->registry.get().registry.at(this->key);
                } else {
                    throw SlokedError(
                        "Registry: Resource points to non-existing object");
                }
            }

            void Release() {
                this->registry.get().Unuse(this->key);
                this->key = SlokedRegistry<T>::None;
            }

         private:
            Resource(Key key, SlokedRegistry<T> &registry)
                : key(key), registry(std::ref(registry)) {
                this->registry.get().Use(this->key);
            }

            Key key;
            std::reference_wrapper<SlokedRegistry<T>> registry;
        };

        SlokedRegistry() : nextKey(1) {}

        Resource Add(std::unique_ptr<T> obj) {
            Key objKey = this->nextKey++;
            this->registry.insert(std::make_pair(objKey, std::move(obj)));
            this->refcount[objKey] = 0;
            return Resource(objKey, *this);
        }

        std::optional<Resource> Get(Key key) {
            if (this->registry.count(key) != 0) {
                return Resource(key, *this);
            } else {
                return {};
            }
        }

        bool Has(Key key) {
            return this->registry.count(key) != 0;
        }

        static constexpr Key None = 0;

     private:
        void Use(Key key) {
            if (this->refcount.count(key) != 0) {
                this->refcount[key]++;
            }
        }

        void Unuse(Key key) {
            if (this->refcount.count(key) != 0) {
                this->refcount[key]--;
                if (this->refcount[key] == 0) {
                    this->registry.erase(key);
                    this->refcount.erase(key);
                }
            }
        }

        Key nextKey;
        std::map<Key, std::unique_ptr<T>> registry;
        std::map<Key, uint32_t> refcount;
    };
}  // namespace sloked

#endif
