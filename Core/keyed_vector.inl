/*****************************************************************//**
 * \file   keyed_vector.inl
 * \brief  Array that has stable indexing and compact memory usage
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include "keyed_vector.hpp"

namespace gtl
{
    template<typename T>
    constexpr size_t keyed_vector<T>::insert(const T& obj)
    {
        if (mAvailableSlots.empty())
        {
            size_t backIndex = mData.size();
            mData.emplace_back(obj);

            return backIndex;
        }

        size_t openIndex = mAvailableSlots.back();
        mAvailableSlots.pop_back();
        mData.at(openIndex).emplace(obj);

        return openIndex;
    }

    template<typename T>
    template<typename... Params>
    constexpr size_t keyed_vector<T>::emplace(Params&&... params)
    {
        if (mAvailableSlots.empty())
        {
            size_t backIndex = mData.size();
            mData.emplace_back(std::in_place, std::forward<Params>(params)...);

            return backIndex;
        }

        size_t openIndex = mAvailableSlots.back();
        mAvailableSlots.pop_back();
        mData.at(openIndex).emplace(std::forward<Params>(params)...);

        return openIndex;
    }

    template<typename T>
    constexpr void keyed_vector<T>::erase(size_t index)
    {
        if (index >= mData.size())
            throw std::out_of_range("keyed_vector::erase() Index out of range");

        if (!mData.at(index).has_value())
            throw std::logic_error("keyed_vector::erase() Index is already empty");

        mAvailableSlots.push_back(index);
        mData.at(index).reset();
    }

    template<typename T>
    constexpr void keyed_vector<T>::clear()
    {
        mData.clear();
        mAvailableSlots.clear();
    }

    template<typename T>
    constexpr T& keyed_vector<T>::at(size_t index)
    {
        if (index >= mData.size())
            throw std::out_of_range("keyed_vector::at() Index out of range");

        if (!mData.at(index).has_value())
            throw std::logic_error("keyed_vector::at() Index is empty");

        return mData.at(index).value();
    }

    template<typename T>
    constexpr const T& keyed_vector<T>::at(size_t index) const
    {
        if (index >= mData.size())
            throw std::out_of_range("keyed_vector::at() Index out of range");

        if (!mData.at(index).has_value())
            throw std::logic_error("keyed_vector::at() Index is empty");

        return mData.at(index).value();
    }

    template<typename T>
    constexpr size_t keyed_vector<T>::size() const
    {
        return mData.size();
    }

    template<typename T>
    constexpr T& keyed_vector<T>::operator[](size_t index)
    {
        return mData[index].value();
    }

    template<typename T>
    constexpr const T& keyed_vector<T>::operator[](size_t index) const
    {
        return mData[index].value();
    }

    template<typename T>
    inline constexpr void keyed_vector<T>::reserve(size_t size)
    {
        mData.reserve(size);
    }

    template<typename T>
    inline constexpr bool keyed_vector<T>::contains(size_t index) const
    {
        // if within bounds and has a value
        return (index < mData.size() && mData.at(index).has_value());
    }

    template<typename T>
    inline constexpr const std::vector<std::optional<T>>& keyed_vector<T>::container() const
    {
        return mData;
    }

    template<typename T>
    constexpr typename keyed_vector<T>::const_iterator keyed_vector<T>::begin() const
    {
        return const_iterator(mData.begin(), mData.end());
    }

    template<typename T>
    constexpr typename keyed_vector<T>::const_iterator keyed_vector<T>::end() const
    {
        return const_iterator(mData.end(), mData.end());
    }

    template<typename T>
    inline constexpr typename keyed_vector<T>::const_iterator keyed_vector<T>::cbegin() const
    {
        return const_iterator(mData.cbegin(), mData.cend());
    }

    template<typename T>
    inline constexpr typename keyed_vector<T>::const_iterator keyed_vector<T>::cend() const
    {
        return const_iterator(mData.cend(), mData.cend());
    }

    template<typename T>
    constexpr typename keyed_vector<T>::iterator keyed_vector<T>::begin()
    {
        return iterator(mData.begin(), mData.end());
    }

    template<typename T>
    constexpr typename keyed_vector<T>::iterator keyed_vector<T>::end()
    {
        return iterator(mData.end(), mData.end());
    }

    template<typename T>
    inline constexpr keyed_vector<T>::iterator& keyed_vector<T>::iterator::operator++()
    {
        ++mIterator;
        ++mIndex;
        skip_empty_slots();
        return *this;
    }

    template<typename T>
    inline constexpr keyed_vector<T>::iterator keyed_vector<T>::iterator::operator++(int)
    {
        iterator temp = *this;
        ++(*this);
        return temp;
    }

    template<typename T>
    inline constexpr std::pair<const uint64_t, typename keyed_vector<T>::iterator::reference> keyed_vector<T>::iterator::operator*() const
    {
        return { mIndex, mIterator->value() };
    }

    template<typename T>
    inline constexpr keyed_vector<T>::iterator::pointer keyed_vector<T>::iterator::operator->() const
    {
        return &mIterator->value();
    }

    template<typename T>
    inline constexpr bool keyed_vector<T>::iterator::operator==(const iterator& other) const
    {
        return mIterator == other.mIterator;
    }

    template<typename T>
    inline constexpr bool keyed_vector<T>::iterator::operator!=(const iterator& other) const
    {
        return mIterator != other.mIterator;
    }

    template<typename T>
    inline void keyed_vector<T>::iterator::skip_empty_slots()
    {
        while (mIterator != mEnd && !mIterator->has_value())
        {
            ++mIterator;
            ++mIndex;
        }
    }

    template<typename T>
    inline constexpr keyed_vector<T>::const_iterator& keyed_vector<T>::const_iterator::operator++()
    {
        ++mIterator;
        ++mIndex;
        skip_empty_slots();
        return *this;
    }

    template<typename T>
    inline constexpr keyed_vector<T>::const_iterator keyed_vector<T>::const_iterator::operator++(int)
    {
        const_iterator temp = *this;
        ++(*this);
        return temp;
    }

    template<typename T>
    inline constexpr std::pair<const uint64_t, typename keyed_vector<T>::const_iterator::reference> keyed_vector<T>::const_iterator::operator*() const
    {
        return { mIndex, mIterator->value() };
    }

    template<typename T>
    inline constexpr keyed_vector<T>::const_iterator::pointer keyed_vector<T>::const_iterator::operator->() const
    {
        return &mIterator->value();
    }

    template<typename T>
    inline constexpr bool keyed_vector<T>::const_iterator::operator==(const const_iterator& other) const
    {
        return mIterator == other.mIterator;
    }

    template<typename T>
    inline constexpr bool keyed_vector<T>::const_iterator::operator!=(const const_iterator& other) const
    {
        return mIterator != other.mIterator;
    }

    template<typename T>
    inline void keyed_vector<T>::const_iterator::skip_empty_slots()
    {
        while (mIterator != mEnd && !mIterator->has_value())
        {
            ++mIterator;
            ++mIndex;
        }
    }
}

