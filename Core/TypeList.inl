/*****************************************************************//**
 * \file   TypeList.inl
 * \brief  impementation of the TypeList
 * 
 * \author Travis Gronvold
 * \date   January 2025
 *********************************************************************/

#pragma once

#include "TypeList.hpp"

namespace Gep
{
    template <typename... Ts>
    template <typename T>
    constexpr bool TypeList<Ts...>::contains()
    {
        return (std::is_same_v<T, Ts> || ...);
    }

    template <typename... Ts>
    template <typename... Us>
    constexpr TypeList<Ts..., Us...> TypeList<Ts...>::operator+(TypeList<Us...>)
    {
        return TypeList<Ts..., Us...>{};
    }

    template <typename... Ts>
    template <template <typename> typename Predicate>
    constexpr auto TypeList<Ts...>::filter() const
    {
        constexpr auto filtered = TypeList<>{} + (std::conditional_t<Predicate<Ts>::value, TypeList<Ts>, TypeList<>>{} + ...);

        return filtered;
    }

    template <typename... Ts>
    template <typename... Us>
    constexpr bool TypeList<Ts...>::operator==(TypeList<Us...> other)
    {
        if (count() != other.count()) return false;

        return (contains<Us>() && ...);
    }

    template <typename... Ts>
    template <typename... Us>
    constexpr bool TypeList<Ts...>::operator!=(TypeList<Us...> other)
    {
        return !(*this == other);
    }

    template <typename... Ts>
    constexpr std::size_t TypeList<Ts...>::count()
    {
        return sizeof...(Ts);
    }

    template <typename... Ts>
    constexpr bool TypeList<Ts...>::empty()
    {
        return count() == 0;
    }

    template <typename... Ts>
    constexpr std::tuple<Ts...> TypeList<Ts...>::to_tuple()
    {
        return std::tuple<Ts...>{};
    }

    template <typename... Ts>
    template <typename... Args>
    constexpr std::tuple<Ts...> TypeList<Ts...>::to_tuple(Args&&... args)
    {
        return std::tuple<Ts...>{static_cast<Ts>(std::forward<Args>(args))...};
    }

    template <typename... Ts>
    template <typename Lambda>
    constexpr void TypeList<Ts...>::for_each(Lambda&& l)
    {
        (l.template operator() < Ts > (), ...);
    }

    template<typename ...Ts>
    template<typename Lambda>
    constexpr void TypeList<Ts...>::for_all(Lambda&& l)
    {
        l.template operator()<Ts... >();
    }
}

