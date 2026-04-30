#pragma once

#include "binary_buffer.hpp"

namespace gtl
{
    template <typename Type>
    inline void binary_buffer::add(const Type &type)
    {
        mBytes.resize(mBytes.size() + sizeof(Type));
        std::memcpy(mBytes.data() + mCursor, &type, sizeof(Type));
        mCursor += sizeof(Type);
    }

    template<>
    inline void binary_buffer::add(const std::string& string)
    {
        // write the size of the string
        add(string.size());

        // write the contents of the string
        mBytes.resize(mBytes.size() + string.size());
        std::memcpy(mBytes.data() + mCursor, string.c_str(), string.size());
        mCursor += string.size();
    }

    template <typename Type>
    inline void binary_buffer::add(const std::vector<Type>& types)
    {
        const size_t dataSize = types.size() * sizeof(Type);

        add(dataSize);

        mBytes.resize(mBytes.size() + dataSize);
        std::memcpy(mBytes.data() + mCursor, types.data(), dataSize);
        mCursor += dataSize;
    }

    template <typename Type, size_t SIZE>
    inline void binary_buffer::add(const std::array<Type, SIZE>& types)
    {
        const size_t dataSize = sizeof(Type) * SIZE;
        mBytes.resize(mBytes.size() + dataSize);
        std::memcpy(mBytes.data() + mCursor, types.data(), dataSize);
        mCursor += dataSize;
    }

    template <typename Type>
    inline void binary_buffer::get(Type& type) const
    {
        if (mCursor >= mBytes.size())
            throw std::out_of_range("get() error");

        std::memcpy(&type, mBytes.data() + mCursor, sizeof(Type));
        mCursor += sizeof(Type);
    }

    template<>
    inline void binary_buffer::get(std::string& string) const
    {
        if (mCursor >= mBytes.size())
            throw std::out_of_range("get() error");

        // read the size of the string
        size_t strSize = 0;
        get(strSize);

        // read the contents
        string.resize(strSize);
        std::memcpy(string.data(), mBytes.data() + mCursor, string.size());
        mCursor += string.size();
    }

    template <typename Type>
    inline void binary_buffer::get(std::vector<Type>& types) const
    {
        if (mCursor >= mBytes.size())
            throw std::out_of_range("get() error");

        size_t dataSize = 0;
        get(dataSize);

        types.resize(dataSize / sizeof(Type));
        std::memcpy(types.data(), mBytes.data() + mCursor, dataSize);
        mCursor += dataSize;
    }

    template <typename Type, size_t SIZE>
    inline void binary_buffer::get(std::array<Type, SIZE>& types) const
    {
        if (mCursor >= mBytes.size())
            throw std::out_of_range("get() error");

        const size_t dataSize = sizeof(Type) * SIZE;
        std::memcpy(types.data(), mBytes.data() + mCursor, dataSize);
        mCursor += dataSize;
    }

}