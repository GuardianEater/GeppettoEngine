/*****************************************************************//**
 * \file   GPUVector.hpp
 * \brief  Handles an ssbo automatically, 
 *         allowing efficient allocation and resizing when needed
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   October 2025
 *********************************************************************/

#pragma once

#include <vector>

namespace Gep
{
    // opengl context doesnt have to exist on construction
    // must call create() before use
    template <typename Type, size_t BINDING_POINT>
    class gpu_vector
    {
    public: // iterators
        using iterator = typename std::vector<Type>::iterator;
        using const_iterator = typename std::vector<Type>::const_iterator;

        iterator begin() { return mCPUBuffer.begin(); }
        iterator end() { return mCPUBuffer.end(); }

        const_iterator begin() const { return mCPUBuffer.begin(); }
        const_iterator end()   const { return mCPUBuffer.end(); }

        const_iterator cbegin() const { return mCPUBuffer.cbegin(); }
        const_iterator cend()   const { return mCPUBuffer.cend(); }

    public: // interface
        gpu_vector();
        ~gpu_vector();

        void create();

        template <typename OtherIterator>
        iterator insert(const_iterator where, OtherIterator first, OtherIterator last);

        template <class... Args>
        iterator emplace(const_iterator where, Args&&... args);

        template <class... Args>
        Type& emplace_back(Args&&... args);

        void push_back(const Type& v);    // inserts a new value into buffer, doesnt touch gpu memory
        void reserve(size_t newCapacity); // reserves the cpu side vector, doesnt touch gpu memory
        void clear();                     // clear cpu side vector, doesnt touch gpu memory

        size_t size() const;     // returns the size of the cpu vector
        size_t capacity() const; // returns the capacity allocated on the gpu

        bool empty();

        void commit(); // moves all data to the gpu all at once, re-allocating the gpu side buffer if needed

        Type* cpu_data() { return mCPUBuffer.data(); }
        const Type* cpu_data() const { return mCPUBuffer.data(); }

        const Type& at(size_t index) const { return mCPUBuffer.at(index); }
        Type& at(size_t index) { return mCPUBuffer.at(index); }

    private:
        void gpu_clear(); // clears the contents on the gpu
        void gpu_reserve(size_t newCapacity); // reallocates the internal buffer, note doesn't copy existing data over, assumed to be overwritten any ways

    private:
        constexpr static GLenum mFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        constexpr static size_t mFirstAllocationSize = 32;

        std::vector<Type> mCPUBuffer; // data currently on the cpu, prior to being commit to the gpu

        GLuint mBufferHandle = 0; // the opengl managed handle to the memory block
        Type* mGPUPointer = nullptr; // pointer directly into gpu memory

        size_t mCapacity = 0; // the current amount of objects allocated for on the gpu
    };
}

#include "GPUVector.inl"