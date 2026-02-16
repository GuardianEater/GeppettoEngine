/*****************************************************************//**
 * \file   Logger.inl
 * \brief  Various logging functions
 *
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include "Logger.hpp"

#include "GetCallerInfo.hpp"

namespace Gep
{
    template <typename... Args>
    void Log::Trace(Args&&... args)
    {
        Gep::CallerInfo ci{}; // unused

        FormatLog(LogLevel::Trace, ci, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log::Info(Args&&... args)
    {
        Gep::CallerInfo ci{}; // unused

        FormatLog(LogLevel::Info, ci, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log::Warning(Args&&... args)
    {
        Gep::CallerInfo ci{}; // unused

        FormatLog(LogLevel::Warning, ci, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log::Important(Args&&... args)
    {
        Gep::CallerInfo ci{}; // unused

        FormatLog(LogLevel::Important, ci, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log::Error(Args&&... args)
    {
        Gep::CallerInfo callerInfo = Gep::GetCallerInfo(_ReturnAddress());

        FormatLog(LogLevel::Error, callerInfo, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log::Critical(Args&&... args)
    {
        Gep::CallerInfo callerInfo = Gep::GetCallerInfo(_ReturnAddress());

        FormatLog(LogLevel::Critical, callerInfo, std::forward<Args>(args)...);
#ifdef _DEBUG
        __debugbreak();
#else // _DEBUG
        throw std::runtime_error("Critical Error"); // check the console!
#endif // _DEBUG
    }

    template<typename Type>
    inline void Log::FormatAdd(std::ostringstream& oss, const Type& type)
    {
        if constexpr (TypeHasStreamInsertion<Type>)
        {
            oss << type;
        }
        else if constexpr (TypeHasIterator<Type>)
        {
            oss << '[';
            bool first = true;
            for (const auto& e : type) 
            {
                if (!first) oss << ',';
                first = false;
                FormatAdd(oss, e);
            }
            oss << ']';
        }
        else
        {
            oss << "[???]";
        }
    }

    template <typename... Args>
    void Log::FormatLog(LogLevel level, const Gep::CallerInfo& caller, Args&&... args)
    {
        std::ostringstream oss;
        oss << "[" << GetCurrentTime() << "] ";

        // prefix errors with caller information
        if (level >= mPrintLevel)
        {
            switch (level)
            {
            case LogLevel::Error:
                oss << "[" << caller.functionName << "] ";
                break;
            case LogLevel::Critical:
                oss << "[" << std::filesystem::path(caller.fileName).filename().string() << ":" << caller.functionName << ":" << caller.lineNumber << "] ";
                break;
            default:
                break;
            }
        }

        (FormatAdd(oss, args), ...);

        // print to console
        if (level >= mPrintLevel)
        {
            switch (level)
            {
            case LogLevel::Trace:
                std::cout << Color::cyan << oss.str() << Color::reset << std::endl;
                break;
            case LogLevel::Info:
                std::cout << Color::green << oss.str() << Color::reset << std::endl;
                break;
            case LogLevel::Warning:
                std::cout << Color::yellow << oss.str() << Color::reset << std::endl;
                break;
            case LogLevel::Error:
                std::cout << Color::red << oss.str() << Color::reset << std::endl;
                break;
            case LogLevel::Important:
                std::cout << Color::blue << oss.str() << Color::reset << std::endl;
                break;
            case LogLevel::Critical:
                std::cout << Color::blackred << oss.str() << Color::reset << std::endl;
                break;
            default:
                break;
            }
        }

        WriteLog(oss.str());
    }
}

