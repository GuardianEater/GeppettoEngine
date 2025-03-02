/*****************************************************************//**
 * \file   KeyedVector.hpp
 * \brief  A vector that returns the index of the inserted object, that key is used to access the object
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
#include <optional>

namespace Gep
{
    template<typename T>
    class keyed_vector
    {
    public:
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using const_reference = const T&;
        using const_pointer = const T*;
        using difference_type = std::ptrdiff_t;

        constexpr keyed_vector() = default;
        constexpr keyed_vector(const keyed_vector&) = default;
        constexpr keyed_vector& operator=(const keyed_vector&) = default;
        constexpr keyed_vector(keyed_vector&&) = default;
        constexpr keyed_vector& operator=(keyed_vector&&) = default;
        ~keyed_vector() = default;

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
        constexpr void reserve(size_t size);
        constexpr bool contains(size_t index) const;

        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;

            using parent_iterator       = typename std::vector<std::optional<T>>::iterator;

            iterator(parent_iterator iterator, parent_iterator end, size_t index = 0)
                : mIterator(iterator)
                , mEnd(end)
                , mIndex(index)
            {
                skip_empty_slots();
            }

            iterator() = default;
            iterator(const iterator&) = default;
            iterator& operator=(const iterator&) = default;
            constexpr iterator& operator++();
            constexpr iterator operator++(int);
            constexpr std::pair<const uint64_t, reference> operator*() const;
            constexpr pointer operator->() const;
            constexpr bool operator==(const iterator& other) const;
            constexpr bool operator!=(const iterator& other) const;

        private:
            parent_iterator mIterator;
            parent_iterator mEnd;
            size_t mIndex;
            void skip_empty_slots();
        };

        class const_iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = const T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;

            using parent_const_iterator = typename std::vector<std::optional<T>>::const_iterator;

            const_iterator(parent_const_iterator iterator, parent_const_iterator end, size_t index = 0)
                : mIterator(iterator)
                , mEnd(end)
                , mIndex(index)
            {
                skip_empty_slots();
            }

            const_iterator() = default;
            const_iterator(const const_iterator&) = default;
            const_iterator& operator=(const const_iterator&) = default;
            constexpr const_iterator& operator++();
            constexpr const_iterator operator++(int);
            constexpr std::pair<const uint64_t, typename keyed_vector<T>::const_iterator::reference> operator*() const;
            constexpr pointer operator->() const;
            constexpr bool operator==(const const_iterator& other) const;
            constexpr bool operator!=(const const_iterator& other) const;

        private:
            parent_const_iterator mIterator;
            parent_const_iterator mEnd;
            size_t mIndex;
            void skip_empty_slots();
        };

        constexpr const_iterator begin() const;
        constexpr const_iterator end() const;
        constexpr const_iterator cbegin() const;
        constexpr const_iterator cend() const;
        constexpr iterator begin();
        constexpr iterator end();

    private:
        std::vector<std::optional<T>> mData; // data never gets removed from the vector, simply leaves a hole
        std::vector<size_t> mAvailableSlots; // indexs to slots that are available
    };
}

#include "KeyedVector.inl"

