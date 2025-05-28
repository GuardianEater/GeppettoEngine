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
}

namespace Gep
{
    template<typename Type>
    void EraseRemove(std::vector<Type>& vec, const Type& value)
    {
        vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
    }
}