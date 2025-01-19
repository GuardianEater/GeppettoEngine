/*****************************************************************//**
 * \file   Logger.cpp
 * \brief  Various logging functions
 *
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#include "Logger.hpp"

namespace Gep
{
    std::mutex Log::mMutex;
    std::unique_ptr<std::ofstream> Log::mFileStream;
    Log::LogLevel Log::mPrintLevel = Log::LogLevel::trace;

    void Log::SetOutputFile(const std::string& filename)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mFileStream = std::make_unique<std::ofstream>(filename, std::ios::app);
    }

    void Log::SetPrintLevel(LogLevel level)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mPrintLevel = level;
    }

    void Log::WriteLog(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mFileStream && mFileStream->is_open())
        {
            (*mFileStream) << message << std::endl;
        }
    }

    std::string Log::GetCurrentTime()
    {
        auto now = std::time(nullptr);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return buf;
    }
}

