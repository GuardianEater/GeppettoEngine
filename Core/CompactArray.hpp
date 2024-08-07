/*****************************************************************//**
 * \file   CompactArray.hpp
 * \brief  Array that has stable indexing and compact memory usage
 * 
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

namespace Gep
{
    template<typename T>
    class compact_array 
    {
    public:
        // Adds an object to the array and returns a stable index
        constexpr size_t insert(const T& obj) 
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
                // reuse an available index
                size_t openIndex = mAvailableIndexs.back();
                mAvailableIndexs.pop_back();

                mUserToData.at(openIndex) = backIndex;
                return openIndex;
            }
        }

        template<typename... Params>
        constexpr size_t emplace(Params&&... params)
        {
            return insert(T(std::forward<Params>(params)...));
        }

        // Removes an object by index
        constexpr void erase(size_t index) 
        {
            assert(!(std::find(mAvailableIndexs.begin(), mAvailableIndexs.end(), index) != mAvailableIndexs.end()) && "Double erase");
            assert(!(index >= mUserToData.size()) && "Index out of range");

            size_t backIndex = mData.size() - 1;
            size_t dataIndex = mUserToData[index];

            mAvailableIndexs.push_back(index);

            // checks if the data index is the last to perform swap and pop
            if (dataIndex != backIndex) 
            {
                std::swap(mData[dataIndex], mData.back());
                mUserToData[backIndex] = dataIndex;
            }
            mData.pop_back();
        }

        constexpr void clear()
        {
            mData.clear();
            mAvailableIndexs.clear();
            mUserToData.clear();
        }

        // Access an object by index
        constexpr T& at(size_t index)
        {
            assert(!(index >= mUserToData.size()) && "Index out of range");

            return mData.at(mUserToData.at(index));
        }

        // Constant access to an object by index
        constexpr const T& at(size_t index) const 
        {
            assert(!(index >= mUserToData.size()) && "Index out of range");

            return mData.at(mUserToData.at(index));
        }

        constexpr size_t size() const
        {
            return mData.size();
        }

        constexpr T& operator[](size_t index)
        {
            return at(index);
        }

        constexpr const T& operator[](size_t index) const
        {
            return at(index);
        }

        using iterator = std::vector<T>::iterator;
        using const_iterator = std::vector<T>::const_iterator;

        constexpr const_iterator begin() const
        {
            return mData.begin();
        }
        
        constexpr const_iterator end() const
        {
            return mData.end();
        }

        constexpr iterator begin()
        {
            return mData.begin();
        }

        constexpr iterator end()
        {
            return mData.end();
        }


    private:
        std::vector<T> mData; // the actual data
        std::vector<size_t> mAvailableIndexs; // list of free user indices
        std::map<size_t, size_t> mUserToData; // maps user indices to data indices
    };
}
