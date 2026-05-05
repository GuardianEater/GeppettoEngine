/*****************************************************************//**
 * \file   binary_serializer.hpp
 * \brief  
 * 
 * \author 2018t
 * \date   April 2026
 *********************************************************************/

#pragma once

#include <vector>
#include <string>

namespace gtl
{
    class binary_buffer
    {
    public:
        static binary_buffer from_file(const std::filesystem::path& path)
        {
            std::ifstream file{ path, std::ios::binary };
            binary_buffer bin;

            if (!file)
                return bin;

            file >> bin;
            return bin;
        }

        // constructors
        binary_buffer() = default;
        ~binary_buffer() = default;
        binary_buffer(binary_buffer&& other) = default;
        binary_buffer& operator=(binary_buffer&& other) = default;
        binary_buffer(const binary_buffer& other) = default;
        binary_buffer& operator=(const binary_buffer& other) = default;

        binary_buffer(const std::vector<std::byte>& bytes)
          : mCursor(0)
          , mBytes(bytes)
        {}
        binary_buffer(std::vector<std::byte>&& bytes)
          : mCursor(0)
          , mBytes(bytes)
        {}

        friend std::ostream& operator<<(std::ostream& os, const binary_buffer& buf)
        {
            os.write(reinterpret_cast<const char*>(buf.mBytes.data()), buf.mBytes.size());
            return os;
        }

        friend std::istream& operator>>(std::istream& is, binary_buffer& buf)
        {
            // get size
            auto pos = is.tellg();
            is.seekg(0, std::ios::end);
            std::streamsize size = is.tellg() - pos;
            is.seekg(pos);

            buf.mBytes.resize(static_cast<size_t>(size));

            // read raw bytes
            is.read(reinterpret_cast<char*>(buf.mBytes.data()), size);

            return is;
        }

        template <typename Type>
        void add(const Type& type);
        template <>
        void add(const std::string& type);
        template <typename Type>
        void add(const std::vector<Type>& types);
        template <typename Type, size_t SIZE>
        void add(const std::array<Type, SIZE>& types);
        template <typename Type>
        void add(const Type* types, size_t count);

        template <typename Type>
        void get(Type& type) const;
        template<>
        void get(std::string& string) const;
        template <typename Type>
        void get(std::vector<Type>& types) const;
        template <typename Type, size_t SIZE>
        void get(std::array<Type, SIZE>& types) const;
        template <typename Type>
        void get(Type* types, size_t count) const;

        void seek(size_t newCursor) const { mCursor = newCursor; }
        size_t size() const { return mBytes.size(); }

        std::byte* data() { return mBytes.data(); }
        const std::byte* data() const { return mBytes.data(); }

    private:
        mutable size_t mCursor = 0;
        std::vector<std::byte> mBytes;
    };
}

#include "binary_buffer.inl"