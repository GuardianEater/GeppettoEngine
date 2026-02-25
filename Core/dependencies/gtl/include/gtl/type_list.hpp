/*****************************************************************//**
 * \file   type_list.hpp
 * \brief  
 * 
 * \author Travis Gronvold
 * \date   January 2025
 *********************************************************************/

#pragma once

#include <type_traits>
#include <tuple>

namespace gtl
{
    template <typename... Ts>
    struct type_list
    {
        // checks if the typelist contains a type
        template <typename T>
        constexpr bool contains();

        // concatenates two typelists together
        template <typename... Us>
        constexpr type_list<Ts..., Us...> operator+(type_list<Us...>);

        // given a predicate that takes a type as a template parameter, returns a new typelist with only the types that satisfy the predicate
        template <template <typename> typename Predicate>
        constexpr auto filter() const;

        // checks if two typelists are the same
        template <typename... Us>
        constexpr bool operator==(type_list<Us...> other);

        // check if two typelists are different
        template <typename... Us>
        constexpr bool operator!=(type_list<Us...> other);

        // gets the amount of types in the typelist
        constexpr std::size_t count();

        // checks if the typelist is empty
        constexpr bool empty();

        // default constructs every type in the typelist inside of a tuple and returns it.
        constexpr std::tuple<Ts...> to_tuple();

        // use a tuple for construction parameters for each corresponding type
        // auto t = types.to_tuple(
        //     std::make_tuple(1),      // constructs Type A with 1
        //     std::make_tuple(2.5f, 3) // constructs Type B with 2.5f and 3
        // );
        template <typename... ConstructionTuples>
        constexpr std::tuple<Ts...> to_tuple(ConstructionTuples&&... args);

        // iterates over each type in the typelist and calls the lambda with that type as a template parameter
        template <typename Lambda>
        constexpr void for_each(Lambda&& l);

        // doesnt iterate, calls the lambda with the entire arg list as template params, similar to std::apply
        template <typename Lambda>
        constexpr void for_all(Lambda&& l);
    };
}

#include "type_list.inl"

