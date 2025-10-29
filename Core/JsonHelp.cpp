/*****************************************************************//**
 * \file   JsonHelp.cpp
 * \brief  implementation of the JsonHelp functions
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   March 2025
 *********************************************************************/

#include "pch.hpp"
#include "JsonHelp.hpp"
#include <glm/gtx/type_trait.hpp>

namespace nlohmann
{
    void to_json(json& j, const glm::vec3& v)
    {
        j = { v.x, v.y, v.z };
    }

    void from_json(const json& j, glm::vec3& v)
    {
        for (size_t i = 0; i < 3; ++i)
            v[i] = j.at(i).get<float>();
    }

    void to_json(json& j, const glm::vec4& v)
    {
        j = { v.x, v.y, v.z, v.w };
    }

    void from_json(const json& j, glm::vec4& v)
    {
        for (size_t i = 0; i < 4; ++i)
            v[i] = j.at(i).get<float>();
    }

    void to_json(json& j, const glm::quat& v)
    {
        j = { v.x, v.y, v.z, v.w };
    }

    void from_json(const json& j, glm::quat& v)
    {
        for (size_t i = 0; i < 4; ++i)
            v[i] = j.at(i).get<float>();
    }

    void to_json(json& j, const glm::mat3& v)
    {
        const float* p = glm::value_ptr(v); // 9 floats, column-major
        j = json::array();
        for (size_t i = 0; i < 9; ++i)
            j.push_back(p[i]);
    }
    
    void from_json(const json& j, glm::mat3& v)
    {
        float* p = glm::value_ptr(v);
        for (size_t i = 0; i < 9; ++i)
            p[i] = j.at(i).get<float>();
    }

    void to_json(json& j, const glm::mat4& v)
    {
        const float* p = glm::value_ptr(v); // 16 floats, column-major
        j = json::array();
        for (size_t i = 0; i < 16; ++i)
            j.push_back(p[i]);
    }

    void from_json(const json& j, glm::mat4& v)
    {
        float* p = glm::value_ptr(v);
        for (size_t i = 0; i < 16; ++i)
            p[i] = j.at(i).get<float>();
    }

    void to_json(json& j, const Gep::UUID& v)
    {
        j = v.ToString();
    }

    void from_json(const json& j, Gep::UUID& v)
    {
        v = Gep::UUID::FromString(j); // json implicit conversion to string
    }
}
