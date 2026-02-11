/*****************************************************************//**
 * \file   STLHelp.hpp
 * \brief  helper functions to wrap common stl operations
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include <vector>

namespace Gep
{
    // performs erase remove idiom on a vector; removes the given value from the vector
    template <typename Type>
    void EraseRemove(std::vector<Type>& vec, const Type& value);

    // if looping is false behaves the same as clamp, however if looping is true, will wrap around if the number goes out of range
    template <typename Type>
        requires std::is_floating_point_v<Type>
    Type WrapOrClamp(const Type value, const Type min, const Type max, bool looping);

    // given a span of items and a function to extract a value from those items, return true if all values are the same.
    template <typename T, typename GetterFunction>
        requires std::is_invocable_v<GetterFunction, const T&> && std::equality_comparable<std::invoke_result_t<GetterFunction, const T&>>
    bool IsUniform(std::span<const T> values, GetterFunction&& get = [](const T& v) { return v; });

    // given a string, replaces all occurences of the "from" string with the "to" string
    void ReplaceAll(std::string& str, const std::string& from, const std::string& to);
}

#include "STLHelp.inl"