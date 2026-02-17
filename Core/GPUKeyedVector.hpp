/*****************************************************************//**
 * \file   GPUKeyedVector.hpp
 * \brief  a keyed vector that automatically handles an ssbo.
 * 
 * \author 2018t
 * \date   February 2026
 *********************************************************************/

#pragma once

#include "GPUVector.hpp"
#include "keyed_vector.hpp"
#include <cstddef>

namespace Gep
{
    template <typename Type, size_t BINDING_POINT>
    class gpu_keyed_vector
    {
    public:
        using PlaceHolderType = std::array<std::byte, sizeof(Type)>;

        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = Type;
            using difference_type = std::ptrdiff_t;
            using reference = Type&;
            using pointer = Type*;

            iterator() = default;

            iterator(gpu_keyed_vector* owner, size_t index)
                : mOwner(owner)
                , mIndex(index)
            {
                skip_empty_slots();
            }

            iterator& operator++()
            {
                ++mIndex;
                skip_empty_slots();
                return *this;
            }

            iterator operator++(int)
            {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            std::pair<const uint64_t, reference> operator*() const
            {
                return { static_cast<uint64_t>(mIndex), mOwner->at(mIndex) };
            }

            pointer operator->() const
            {
                return &mOwner->at(mIndex);
            }

            bool operator==(const iterator& other) const { return mOwner == other.mOwner && mIndex == other.mIndex; }
            bool operator!=(const iterator& other) const { return !(*this == other); }

        private:
            gpu_keyed_vector* mOwner = nullptr;
            size_t mIndex = 0;

            void skip_empty_slots()
            {
                if (!mOwner) return;

                const size_t n = mOwner->mData.size();
                while (mIndex < n && !mOwner->contains(mIndex))
                    ++mIndex;
            }
        };

        class const_iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = const Type;
            using difference_type = std::ptrdiff_t;
            using reference = const Type&;
            using pointer = const Type*;

            const_iterator() = default;

            const_iterator(const gpu_keyed_vector* owner, size_t index)
                : mOwner(owner)
                , mIndex(index)
            {
                skip_empty_slots();
            }

            const_iterator& operator++()
            {
                ++mIndex;
                skip_empty_slots();
                return *this;
            }

            const_iterator operator++(int)
            {
                const_iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            std::pair<const uint64_t, reference> operator*() const
            {
                return { static_cast<uint64_t>(mIndex), mOwner->at(mIndex) };
            }

            pointer operator->() const
            {
                return &mOwner->at(mIndex);
            }

            bool operator==(const const_iterator& other) const { return mOwner == other.mOwner && mIndex == other.mIndex; }
            bool operator!=(const const_iterator& other) const { return !(*this == other); }

        private:
            const gpu_keyed_vector* mOwner = nullptr;
            size_t mIndex = 0;

            void skip_empty_slots()
            {
                if (!mOwner) return;

                const size_t n = mOwner->mData.size();
                while (mIndex < n && !mOwner->contains(mIndex))
                    ++mIndex;
            }
        };

        iterator begin() { return iterator(this, 0); }
        iterator end() { return iterator(this, mData.size()); }

        const_iterator begin() const { return const_iterator(this, 0); }
        const_iterator end() const { return const_iterator(this, mData.size()); }

        const_iterator cbegin() const { return const_iterator(this, 0); }
        const_iterator cend() const { return const_iterator(this, mData.size()); }

    public:

        template <typename... Args>
        size_t emplace(Args&&... args)
        {
            // if there are no open slots, insert at the back of the vector
            if (mAvailableSlots.empty())
            {
                size_t backIndex = mData.size();
                PlaceHolderType& placeHolder = mData.emplace_back();
                Type* dst = reinterpret_cast<Type*>(&placeHolder);
                mOccupiedSlots.emplace_back(1); // mark the slot as occupied
                new (dst) Type(std::forward<Args>(args)...);

                return backIndex;
            }

            // if there are open slots, use the most recently freed one
            size_t openIndex = mAvailableSlots.back();
            mAvailableSlots.pop_back();
            PlaceHolderType& placeHolder = mData.at(openIndex);
            Type* dst = reinterpret_cast<Type*>(&placeHolder);
            mOccupiedSlots.at(openIndex) = 1; // mark the slot as occupied
            new (dst) Type(std::forward<Args>(args)...);

            return openIndex;
        }

        void erase(size_t index)
        {
            if (!contains(index))
                throw std::logic_error("gpu_keyed_vector::erase() Index is already empty");

            // add the index to the stack of available slots
            mAvailableSlots.push_back(index);
            mOccupiedSlots.at(index) = 0; // mark the slot as empty

            // call the destructor of the object at that slot
            PlaceHolderType& placeHolder = mData.at(index);
            reinterpret_cast<Type*>(&placeHolder)->~Type();
        }

        void create()
        {
            mData.create();
        }

        void commit()
        {
            mData.commit();
        }

        void clear()
        {
            mData.clear();
        }

        const Type& at(size_t index) const
        {
            if (!contains(index))
                throw std::logic_error("gpu_keyed_vector::at() Index is empty");

            PlaceHolderType& placeHolder = mData.at(index);

            return *reinterpret_cast<Type*>(&placeHolder);
        }

        Type& at(size_t index)
        {
            if (!contains(index))
                throw std::logic_error("gpu_keyed_vector::at() Index is empty");

            PlaceHolderType& placeHolder = mData.at(index);

            return *reinterpret_cast<Type*>(&placeHolder);
        }
    
        size_t size() const
        {
            return mData.size();
        }
    
        bool contains(size_t index) const
        {
            // if within bounds and has a value
            return (index < mData.size() && mOccupiedSlots.at(index) == 1);
        }

        size_t capacity() const
        {
            return mData.capacity();
        }

    private:
        std::vector<uint64_t> mAvailableSlots; // stack of available slots, if empty then insert at the back
        std::vector<uint8_t> mOccupiedSlots; // 1 if the slot is occupied, 0 if it is empty

        gpu_vector<PlaceHolderType, BINDING_POINT> mData;
    };
}
