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
        if (mGPUPointer)
            glUnmapNamedBuffer(mBufferHandle);
        glDeleteBuffers(1, &mBufferHandle);
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
            if (mGPUPointer)
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
        return mCPUBuffer.emplace(where, args);
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
            Gep::Log::Important("expanding binding point [", BINDING_POINT, "] to a size of [", mCapacity == 0 ? mFirstAllocationSize : mCapacity * 2, "]");
            gpu_reserve(mCapacity == 0 ? mFirstAllocationSize : mCapacity * 2); // double amount of memory on gpu
        }

        // Check if mapping failed
        if (mGPUPointer == nullptr)
        {
            // Log OpenGL error or handle failure
            GLenum error = glGetError();
            // Handle the error appropriately
            Gep::Log::Error(error);

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
