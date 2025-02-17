/*****************************************************************//**
 * \file   BinarySerializer.hpp
 * \brief  will write items to a binary file
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include "Logger.hpp"

namespace Gep
{
    template <typename T>
    concept IsIterable = requires(const T& t)
    {
        t.begin();
        t.end();
        t.size();
    };

    class Serializer
    {
    public:

        enum class Mode : char
        {
            None,
            Write,
            Read,
        };

        template <typename T>
        static void RegisterSchema()
        {

        }

        static void SetOutStream(std::ostream& stream)
        {
            mOutStream = &stream;
        }

        static void SetInStream(std::istream& stream)
        {
            mInStream = &stream;
        }

        // sets the serialization mode, calls to serialize will either write data or read in data
        static void SetMode(Mode mode)
        {
            mMode = mode;
        }

        template <typename T>
        static void Serialize(T& t)
        {
            if (mMode == Mode::Write)
            {
                Write(t);
            }
            else if (mMode == Mode::Read)
            {
                Read(t);
            }
            else
            {
                Log::Error("Serializer::Serialize() failed, mode not set");
            }
        }

    private:
        template <typename T>
        static void Write(const T& t)
        {
            if (!mOutStream)
            {
                Log::Error("Serializer::Write() failed, stream is not open");
                return;
            }

            mOutStream->write(reinterpret_cast<const char*>(&t), sizeof(T));
        }

        template <typename T>
        requires IsIterable<T>
        static void Write(const T& t)
        {
            if (!mOutStream)
            {
                Log::Error("Serializer::Write() failed, stream is not open");
                return;
            }

            Write(t.size());
            for (const T& item : t)
            {
                Write(item);
            }
        }

        template <typename T>
        static void Read(T& t)
        {
            if (!mInStream)
            {
                Log::Error("Serializer::Read() failed, stream is not open");
                return;
            }

            mInStream->read(reinterpret_cast<char*>(&t), sizeof(T));
        }

        template <typename T>
        static void Read(std::vector<T>& t)
        {
            if (!mInStream)
            {
                Log::Error("Serializer::Read() failed, stream is not open");
                return;
            }

            size_t size;
            Read(size);
            t.resize(size);
            for (T& item : t)
            {
                Read(item);
            }
        }

        static void Read(std::string& t)
        {
            if (!mInStream)
            {
                Log::Error("Serializer::Read() failed, stream is not open");
                return;
            }

            size_t size;
            Read(size);
            t.resize(size);
            mInStream->read(t.data(), size);
        }

    private:
        static Mode mMode;

        static std::ostream* mOutStream;
        static std::istream* mInStream;
    };
}
