/*****************************************************************//**
 * \file   reflect-cpp-extra.hpp
 * \brief  a bunch of extra overloads for the reflect-cpp library
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include <rfl.hpp>
#include <glm.hpp>

namespace rfl 
{
    template <>
    struct Reflector<glm::vec3> 
    {
        struct ReflType {
            float x, y, z;
        };

        static glm::vec3 to(const ReflType& v) noexcept {
            return { v.x, v.y, v.z };
        }

        static ReflType from(const glm::vec3& v) {
            return { v.x, v.y, v.z };
        }
    };
}
