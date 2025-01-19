/*****************************************************************//**
 * \file   Logger.inl
 * \brief  Various logging functions
 *
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include "Logger.hpp"

namespace Gep
{
    template <typename... Args>
    void Log::Trace(Args&&... args)
    {
        FormatLog(LogLevel::trace, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log::Info(Args&&... args)
    {
        FormatLog(LogLevel::info, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log::Warning(Args&&... args)
    {
        FormatLog(LogLevel::warning, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log::Important(Args&&... args)
    {
        FormatLog(LogLevel::important, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log::Error(Args&&... args)
    {
        FormatLog(LogLevel::error, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log::Critical(Args&&... args)
    {
        FormatLog(LogLevel::critical, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Log::FormatLog(LogLevel level, Args&&... args)
    {
        std::ostringstream oss;
        oss << "[" << GetCurrentTime() << "] [" << rfl::enum_to_string(level) << "] ";
        (oss << ... << args);

        if (level >= mPrintLevel)
        {
            switch (level)
            {
            case LogLevel::trace:
                std::cout << Color::cyan << oss.str() << Color::reset << std::endl;
                break;
            case LogLevel::info:
                std::cout << Color::green << oss.str() << Color::reset << std::endl;
                break;
            case LogLevel::warning:
                std::cout << Color::yellow << oss.str() << Color::reset << std::endl;
                break;
            case LogLevel::error:
                std::cout << Color::red << oss.str() << Color::reset << std::endl;
                break;
            case LogLevel::important:
                std::cout << Color::blue << oss.str() << Color::reset << std::endl;
                break;
            case LogLevel::critical:
                std::cout << Color::blackred << oss.str() << Color::reset << std::endl;
                break;
            default:
                break;
            }
        }

        WriteLog(oss.str());
    }
}

