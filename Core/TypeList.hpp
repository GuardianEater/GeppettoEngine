/*****************************************************************//**
 * \file   TypeList.hpp
 * \brief  
 * 
 * \author Travis Gronvold
 * \date   January 2025
 *********************************************************************/

#pragma once

#include <type_traits>
#include <tuple>

namespace Gep
{
    template <typename... Ts>
    struct type_list
    {
        template <typename T>
        constexpr bool contains();

        template <typename... Us>
        constexpr type_list<Ts..., Us...> operator+(type_list<Us...>);

        template <template <typename> typename Predicate>
        constexpr auto filter() const;

        template <typename... Us>
        constexpr bool operator==(type_list<Us...> other);

        template <typename... Us>
        constexpr bool operator!=(type_list<Us...> other);

        constexpr std::size_t count();

        constexpr bool empty();

        constexpr std::tuple<Ts...> to_tuple();

        template <typename... Args>
        constexpr std::tuple<Ts...> to_tuple(Args&&... args);

        template <typename Lambda>
        constexpr void for_each(Lambda&& l);
    };
}

#include "TypeList.inl"

