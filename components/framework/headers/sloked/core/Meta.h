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

#ifndef SLOKED_CORE_META_H_
#define SLOKED_CORE_META_H_

#include <tuple>
#include <type_traits>
#include <variant>

#include "sloked/Base.h"

namespace sloked {

    template <typename T>
    struct SlokedFunctionTraits
        : public SlokedFunctionTraits<decltype(&T::operator())> {};

    template <typename Return, typename... Args>
    struct SlokedFunctionTraits<Return(Args...)> {
        using Result = Return;

        static constexpr std::size_t ArgumentCount = sizeof...(Args);

        template <std::size_t Index>
        struct Argument {
            static_assert(Index < ArgumentCount);
            using Type =
                typename std::tuple_element<Index, std::tuple<Args...>>::type;
        };
    };

    template <typename Return, typename... Args>
    struct SlokedFunctionTraits<Return (*)(Args...)>
        : public SlokedFunctionTraits<Return(Args...)> {};

    template <typename Class, typename Return, typename... Args>
    struct SlokedFunctionTraits<Return (Class::*)(Args...)>
        : public SlokedFunctionTraits<Return(Args...)> {};

    template <typename Class, typename Return, typename... Args>
    struct SlokedFunctionTraits<Return (Class::*)(Args...) const>
        : public SlokedFunctionTraits<Return(Args...)> {};

    template <typename... T>
    struct Typelist {};

    template <typename H, typename... T>
    struct Typelist<H, T...> {
        using Head = H;
        using Tail = Typelist<T...>;
    };

    template <typename... T>
    struct TypelistContains;

    template <typename Type>
    struct TypelistContains<Type> {
        static constexpr bool Value = false;
    };

    template <typename Type, typename ListHead>
    struct TypelistContains<Type, ListHead> {
        static constexpr bool Value = std::is_same_v<Type, ListHead>;
    };

    template <typename Type, typename ListHead, typename... ListTail>
    struct TypelistContains<Type, ListHead, ListTail...> {
        static constexpr bool Value =
            std::is_same_v<Type, ListHead> ||
            TypelistContains<Type, ListTail...>::Value;
    };

    template <typename UnfilteredList, typename... FilteredTypes>
    struct UniqueVariant {
        using Result = std::conditional_t<
            TypelistContains<typename UnfilteredList::Head,
                             FilteredTypes...>::Value,
            typename UniqueVariant<typename UnfilteredList::Tail,
                                   FilteredTypes...>::Result,
            typename UniqueVariant<typename UnfilteredList::Tail,
                                   FilteredTypes...,
                                   typename UnfilteredList::Head>::Result>;
    };

    template <typename... FilteredTypes>
    struct UniqueVariant<Typelist<>, FilteredTypes...> {
        using Result = std::variant<FilteredTypes...>;
    };

    template <typename... TypeList>
    using UniqueVariant_t =
        typename UniqueVariant<Typelist<TypeList...>>::Result;

    template <typename Fn, typename... T>
    auto BindFirst(Fn &&callable, T &&... args1) {
        return [=](auto &&... args2) {
            return callable(args1..., std::forward<decltype(args2)>(args2)...);
        };
    }

    template <template <typename...> class, typename...>
    struct IsInstantiation : public std::false_type {};

    template <template <typename...> class U, typename... T>
    struct IsInstantiation<U, U<T...>> : public std::true_type {};

}  // namespace sloked

#endif