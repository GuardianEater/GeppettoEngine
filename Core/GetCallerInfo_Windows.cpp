/*****************************************************************//**
 * \file   GetCallerInfo_Windows.cpp
 * \brief  gets the caller information for windows
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#include "pch.hpp"

#ifdef _WIN32

#include "GetCallerInfo.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <intrin.h>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")


namespace Gep
{
    CallerInfo GetCallerInfo(void* returnAddress)
    {
        CallerInfo info;

        HANDLE process = GetCurrentProcess();
        SymInitialize(process, nullptr, TRUE);

        DWORD64 displacement = 0;
        DWORD64 dwAddress = reinterpret_cast<DWORD64>(returnAddress);

        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)]{};
        PSYMBOL_INFO pSymbol = reinterpret_cast<PSYMBOL_INFO>(buffer);

        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;

        bool success = SymFromAddr(process, dwAddress, &displacement, pSymbol);
        if (success)
        {
            IMAGEHLP_LINE64 lineInfo{};
            lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD displacement2 = 0;

            success = SymGetLineFromAddr64(process, dwAddress, &displacement2, &lineInfo);
            if (success)
            {
                info.fileName = lineInfo.FileName;
                info.functionName = pSymbol->Name;
                info.lineNumber = lineInfo.LineNumber;
            }
        }

        return info;
    }
}

#endif // _WIN32
