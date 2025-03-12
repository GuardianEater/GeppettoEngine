/*****************************************************************//**
 * \file   GetCallerInfo.hpp
 * \brief  interface for getting caller information
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include <string>

namespace Gep
{
    struct CallerInfo // aquires information form the calling function
    {
        std::string fileName;
        std::string functionName;
        size_t lineNumber = 0;
    };

    inline void* GetReturnAddress()
    {
#ifdef _WIN32
        return _ReturnAddress();
#elif __linux__
        return __builtin_return_address(0);
#endif
    }

    CallerInfo GetCallerInfo(void* returnAddress);
}
