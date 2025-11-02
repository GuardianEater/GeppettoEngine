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
}

namespace Gep
{
    template<typename Type>
    void EraseRemove(std::vector<Type>& vec, const Type& value)
    {
        vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
    }

    template <typename Type>
        requires std::is_floating_point_v<Type>
    Type WrapOrClamp(const Type value, const Type min, const Type max, bool looping)
    {
        Type result{};

        if (looping)
        {
            result = std::fmod(value, max);
            if (result < min) result += max;
        }
        else
        {
            result = std::clamp(value, min, max);
        }

        return result;
    }
}