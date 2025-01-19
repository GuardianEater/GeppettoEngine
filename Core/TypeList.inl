/*****************************************************************//**
 * \file   TypeList.inl
 * \brief  impementation of the type_list
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
    constexpr bool type_list<Ts...>::contains()
    {
        return (std::is_same_v<T, Ts> || ...);
    }

    template <typename... Ts>
    template <typename... Us>
    constexpr type_list<Ts..., Us...> type_list<Ts...>::operator+(type_list<Us...>)
    {
        return type_list<Ts..., Us...>{};
    }

    template <typename... Ts>
    template <template <typename> typename Predicate>
    constexpr auto type_list<Ts...>::filter() const
    {
        constexpr auto filtered = type_list<>{} + (std::conditional_t<Predicate<Ts>::value, type_list<Ts>, type_list<>>{} + ...);

        return filtered;
    }

    template <typename... Ts>
    template <typename... Us>
    constexpr bool type_list<Ts...>::operator==(type_list<Us...> other)
    {
        if (count() != other.count()) return false;

        return (contains<Us>() && ...);
    }

    template <typename... Ts>
    template <typename... Us>
    constexpr bool type_list<Ts...>::operator!=(type_list<Us...> other)
    {
        return !(*this == other);
    }

    template <typename... Ts>
    constexpr std::size_t type_list<Ts...>::count()
    {
        return sizeof...(Ts);
    }

    template <typename... Ts>
    constexpr bool type_list<Ts...>::empty()
    {
        return count() == 0;
    }

    template <typename... Ts>
    constexpr std::tuple<Ts...> type_list<Ts...>::to_tuple()
    {
        return std::tuple<Ts...>{};
    }

    template <typename... Ts>
    template <typename... Args>
    constexpr std::tuple<Ts...> type_list<Ts...>::to_tuple(Args&&... args)
    {
        return std::tuple<Ts...>{static_cast<Ts>(std::forward<Args>(args))...};
    }

    template <typename... Ts>
    template <typename Lambda>
    constexpr void type_list<Ts...>::for_each(Lambda&& l)
    {
        (l.template operator() < Ts > (), ...);
    }
}

