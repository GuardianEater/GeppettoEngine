/*****************************************************************//**
 * \file   StringHelp.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#include "pch.hpp"
#include "StringHelp.hpp"

namespace Gep
{
    void ToUpper(std::string& str)
    {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::toupper(c)); 
        });
    }

    void ToLower(std::string& str)
    {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c)); 
        });
    }
}
