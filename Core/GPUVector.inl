/*****************************************************************//**
 * \file   GPUVector.inl
 * \brief  implementation for GPU vector
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   October 2025
 *********************************************************************/

#pragma once
#include "GPUVector.hpp"
#include "glew.h"

namespace Gep
{
    template<typename Type, size_t BINDING_POINT>
    inline gpu_vector<Type, BINDING_POINT>::gpu_vector()
    {
    }

    template<typename Type, size_t BINDING_POINT>
    inline gpu_vector<Type, BINDING_POINT>::~gpu_vector()
    {
        gpu_clear();
    }

    template<typename Type, size_t BINDING_POINT>
    inline void gpu_vector<Type, BINDING_POINT>::create()
    {
        glGenBuffers(1, &mBufferHandle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINT, mBufferHandle);
    }

    template<typename Type, size_t BINDING_POINT>
    inline void gpu_vector<Type, BINDING_POINT>::gpu_clear()
    {
        if (mBufferHandle)
        {
            glUnmapNamedBuffer(mBufferHandle);
            glDeleteBuffers(1, &mBufferHandle);
            mBufferHandle = 0;
            mGPUPointer = nullptr;
        }
    }

    template<typename Type, size_t BINDING_POINT>
    template<typename OtherIterator>
    inline gpu_vector<Type, BINDING_POINT>::iterator gpu_vector<Type, BINDING_POINT>::insert(const_iterator where, OtherIterator first, OtherIterator last)
    {
        return mCPUBuffer.insert(where, first, last);
    }

    template<typename Type, size_t BINDING_POINT>
    template<class ...Args>
    inline gpu_vector<Type, BINDING_POINT>::iterator gpu_vector<Type, BINDING_POINT>::emplace(const_iterator where, Args && ...args)
    {
        return mCPUBuffer.emplace(where, std::forward<Args>(args)...);
    }

    template<typename Type, size_t BINDING_POINT>
    template<class ...Args>
    inline Type& gpu_vector<Type, BINDING_POINT>::emplace_back(Args && ...args)
    {
        return mCPUBuffer.emplace_back(std::forward<Args>(args)...);
    }

    template<typename Type, size_t BINDING_POINT>
    inline void gpu_vector<Type, BINDING_POINT>::push_back(const Type& v)
    {
        mCPUBuffer.push_back(v);
    }

    template <typename Type, size_t BINDING_POINT>
    inline void gpu_vector<Type, BINDING_POINT>::reserve(size_t newCapacity)
    {
        mCPUBuffer.reserve(newCapacity);
    }

    template <typename Type, size_t BINDING_POINT>
    inline void gpu_vector<Type, BINDING_POINT>::clear()
    {
        mCPUBuffer.clear();
    }

    template <typename Type, size_t BINDING_POINT>
    inline size_t gpu_vector<Type, BINDING_POINT>::size() const
    {
        return mCPUBuffer.size();
    }

    template <typename Type, size_t BINDING_POINT>
    inline size_t gpu_vector<Type, BINDING_POINT>::capacity() const
    {
        return mCapacity;
    }

    template<typename Type, size_t BINDING_POINT>
    inline void gpu_vector<Type, BINDING_POINT>::commit()
    {
        if (mCPUBuffer.empty())
            return; // do nothing if there is nothing to do

        // if the current amount of objects in the buffer exceeds the amount on the gpu, allocate more memory on the gpu
        if (mCPUBuffer.size() > mCapacity)
        {
            // allocating based on the amount of objects to prevent under allocation from bulk adding
            size_t allocationSize = std::bit_ceil(mCPUBuffer.size()) * 2; // gets the next power of 2
            gpu_reserve(allocationSize);
        }

        // check if mapping failed
        if (mGPUPointer == nullptr)
        {
            GLenum error = glGetError();
            Gep::Log::Error("Failed to map buffer at binding point [", BINDING_POINT, "] glError = [", error, "]");

            return;
        }

        std::memcpy(mGPUPointer, mCPUBuffer.data(), mCPUBuffer.size() * sizeof(Type));
    }

    template<typename Type, size_t BINDING_POINT>
    inline void gpu_vector<Type, BINDING_POINT>::gpu_reserve(size_t newCapacity)
    {
        if (newCapacity <= mCapacity)
            return;

        gpu_clear();
        create();

        // Allocate new buffer with persistent mapping
        glNamedBufferStorage(mBufferHandle, newCapacity * sizeof(Type), nullptr, mFlags | GL_DYNAMIC_STORAGE_BIT);

        mGPUPointer = reinterpret_cast<Type*>(glMapNamedBufferRange(mBufferHandle, 0, newCapacity * sizeof(Type), mFlags));

        mCapacity = newCapacity;
    }
}
