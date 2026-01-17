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
    struct TypeList
    {
        template <typename T>
        constexpr bool contains();

        template <typename... Us>
        constexpr TypeList<Ts..., Us...> operator+(TypeList<Us...>);

        template <template <typename> typename Predicate>
        constexpr auto filter() const;

        template <typename... Us>
        constexpr bool operator==(TypeList<Us...> other);

        template <typename... Us>
        constexpr bool operator!=(TypeList<Us...> other);

        constexpr std::size_t count();

        constexpr bool empty();

        constexpr std::tuple<Ts...> to_tuple();

        template <typename... Args>
        constexpr std::tuple<Ts...> to_tuple(Args&&... args);

        // iterates over each type in the typelist and calls the lambda with that type as a template parameter
        template <typename Lambda>
        constexpr void for_each(Lambda&& l);

        // doesnt iterate, calls the lambda with the entire arg list as template params
        template <typename Lambda>
        constexpr void for_all(Lambda&& l);
    };
}

#include "TypeList.inl"

