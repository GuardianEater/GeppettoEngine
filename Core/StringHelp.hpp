/*****************************************************************//**
 * \file   StringHelp.hpp
 * \brief  various string helper functions
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include <string>

namespace Gep
{
    // converts the entire string to upper case
    void ToUpper(std::string& str);

    // converts the entire string to lower case
    void ToLower(std::string& str);
}
