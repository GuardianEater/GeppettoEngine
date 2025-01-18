/*****************************************************************//**
 * \file   CompactArray.hpp
 * \brief  Array that has stable indexing and compact memory usage
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include <vector>
#include <map>
#include <algorithm>
#include <cassert>

namespace Gep
{
    template<typename T>
    class compact_array
    {
    public:
        constexpr size_t insert(const T& obj);
        template<typename... Params>
        constexpr size_t emplace(Params&&... params);
        constexpr void erase(size_t index);
        constexpr void clear();
        constexpr T& at(size_t index);
        constexpr const T& at(size_t index) const;
        constexpr size_t size() const;
        constexpr T& operator[](size_t index);
        constexpr const T& operator[](size_t index) const;

        using iterator = typename std::vector<T>::iterator;
        using const_iterator = typename std::vector<T>::const_iterator;

        constexpr const_iterator begin() const;
        constexpr const_iterator end() const;
        constexpr iterator begin();
        constexpr iterator end();

    private:
        std::vector<T> mData;
        std::vector<size_t> mAvailableIndexs;
        std::map<size_t, size_t> mUserToData;
    };
}

#include "CompactArray.inl"

