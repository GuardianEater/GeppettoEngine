/*****************************************************************//**
 * \file   CompactArray.inl
 * \brief  Array that has stable indexing and compact memory usage
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

namespace Gep
{
    template<typename T>
    constexpr size_t compact_array<T>::insert(const T& obj)
    {
        size_t backIndex = mData.size();
        mData.push_back(obj);

        if (mAvailableIndexs.empty())
        {
            mUserToData[backIndex] = backIndex;
            return backIndex;
        }
        else
        {
            size_t openIndex = mAvailableIndexs.back();
            mAvailableIndexs.pop_back();

            mUserToData.at(openIndex) = backIndex;
            return openIndex;
        }
    }

    template<typename T>
    template<typename... Params>
    constexpr size_t compact_array<T>::emplace(Params&&... params)
    {
        return insert(T(std::forward<Params>(params)...));
    }

    template<typename T>
    constexpr void compact_array<T>::erase(size_t index)
    {
        assert(!(std::find(mAvailableIndexs.begin(), mAvailableIndexs.end(), index) != mAvailableIndexs.end()) && "Double erase");
        assert(!(index >= mUserToData.size()) && "Index out of range");

        size_t backIndex = mData.size() - 1;
        size_t dataIndex = mUserToData[index];

        mAvailableIndexs.push_back(index);

        if (dataIndex != backIndex)
        {
            std::swap(mData[dataIndex], mData.back());
            mUserToData[backIndex] = dataIndex;
        }
        mData.pop_back();
    }

    template<typename T>
    constexpr void compact_array<T>::clear()
    {
        mData.clear();
        mAvailableIndexs.clear();
        mUserToData.clear();
    }

    template<typename T>
    constexpr T& compact_array<T>::at(size_t index)
    {
        assert(!(index >= mUserToData.size()) && "Index out of range");

        return mData.at(mUserToData.at(index));
    }

    template<typename T>
    constexpr const T& compact_array<T>::at(size_t index) const
    {
        assert(!(index >= mUserToData.size()) && "Index out of range");

        return mData.at(mUserToData.at(index));
    }

    template<typename T>
    constexpr size_t compact_array<T>::size() const
    {
        return mData.size();
    }

    template<typename T>
    constexpr T& compact_array<T>::operator[](size_t index)
    {
        return at(index);
    }

    template<typename T>
    constexpr const T& compact_array<T>::operator[](size_t index) const
    {
        return at(index);
    }

    template<typename T>
    constexpr typename compact_array<T>::const_iterator compact_array<T>::begin() const
    {
        return mData.begin();
    }

    template<typename T>
    constexpr typename compact_array<T>::const_iterator compact_array<T>::end() const
    {
        return mData.end();
    }

    template<typename T>
    constexpr typename compact_array<T>::iterator compact_array<T>::begin()
    {
        return mData.begin();
    }

    template<typename T>
    constexpr typename compact_array<T>::iterator compact_array<T>::end()
    {
        return mData.end();
    }
}

