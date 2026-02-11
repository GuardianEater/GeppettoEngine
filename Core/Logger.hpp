/*****************************************************************//**
 * \file   Logger.hpp
 * \brief  Various logging functions
 *
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <mutex>

#include "GetCallerInfo.hpp"

namespace Gep
{
    namespace Color
    {
        static const std::string reset    = "\033[0m";
        static const std::string red      = "\033[31m";
        static const std::string green    = "\033[32m";
        static const std::string yellow   = "\033[33m";
        static const std::string blue     = "\033[34m";
        static const std::string magenta  = "\033[35m";
        static const std::string cyan     = "\033[36m";
        static const std::string blackred = "\033[30m\033[41m";
    }

    template <typename Type>
    concept TypeHasIterator = requires(const Type & type)
    {
        { type.begin() };
        { type.end() } ;
    };
    
    template <typename Type>
    concept TypeHasStreamInsertion = requires(std::ostream& s, const Type & type)
    {
        { s << type };
    };

    class Log
    {
    public:
        enum class LogLevel : char
        {
            Trace,
            Info,
            Warning,
            Error,
            Important,
            Critical,
        };

        // Set output file, if no output file is set, no file will be written to
        static void SetOutputFile(const std::string& filename);

        // will only print messages with a level greater than or equal to the set level, all messages will still be written to the log file
        static void SetPrintLevel(LogLevel level);

        // Convenience methods for specific log levels
        template <typename... Args> static void Trace(Args&&... args);
        template <typename... Args> static void Info(Args&&... args);
        template <typename... Args> static void Warning(Args&&... args);
        template <typename... Args> static void Error(Args&&... args);
        template <typename... Args> static void Important(Args&&... args);
        template <typename... Args> static void Critical(Args&&... args);

    private:
        static std::mutex mMutex; // mutex for access to the mFileStream
        static std::unique_ptr<std::ofstream> mFileStream; // file stream for the log file, if no output file is set this will be nullptr
        static LogLevel mPrintLevel; // not thread safe, should only be set at the start of program. specifices which level of logs should be output to cout

        // adds an item to the given ostringstream as a string
        template <typename Type>
        static void FormatAdd(std::ostringstream& oss, const Type& type);

        // outputs to std::cout and the log file with the given level, caller info, and message
        template <typename... Args>
        static void FormatLog(LogLevel level, const Gep::CallerInfo& caller, Args&&... args);

        // writes a message to the log file, does not print to std::cout
        static void WriteLog(const std::string& message);

        // formated time when the log message was created
        static std::string GetCurrentTime();
    };
}

#include "Logger.inl"

