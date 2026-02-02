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

    std::string FormatBytes(size_t bytes)
    {
        constexpr std::string_view suffixes[] = { "B", "KB", "MB", "GB", "TB" };

        size_t suffixIndex = 0;
        double bytesD = static_cast<double>(bytes);
        while (bytesD >= 1024 && suffixIndex < std::size(suffixes) - 1)
        {
            bytesD /= 1024;
            ++suffixIndex;
        }

        // only show two decimal places
        return std::format("{:.2f} {}", bytesD, suffixes[suffixIndex]);
    }
}
