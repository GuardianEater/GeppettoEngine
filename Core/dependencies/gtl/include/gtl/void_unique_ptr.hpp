/*****************************************************************//**
 * \file   void_unique_ptr.hpp
 * \brief  A unique pointer that can hold a pointer to any type.
 * 
 * \author 2018t
 * \date   February 2026
 *********************************************************************/

#pragma once

#include <memory>

namespace gtl
{
    using void_unique_ptr = std::unique_ptr<void, void(*)(void*)>;

    template <typename T, typename... Args>
    void_unique_ptr make_unique_void(Args&&... args)
    {
        std::unique_ptr<T> ptr = std::make_unique<T>(std::forward<Args>(args)...);

        return void_unique_ptr(ptr.release(), [](void* p) { delete static_cast<T*>(p); });
    }
} // namespace gtl
