/*****************************************************************//**
 * \file   JsonHelp.cpp
 * \brief  implementation of the JsonHelp functions
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   March 2025
 *********************************************************************/

#include "pch.hpp"
#include "JsonHelp.hpp"

namespace Gep::Json
{
    void WriteType(nlohmann::json& json, const std::string_view name, const glm::vec3& t)
    {
        json[name] = {
            {"x", t.x},
            {"y", t.y},
            {"z", t.z}
        };
    }

    void WriteType(nlohmann::json& json, const std::string_view name, const glm::vec4& t)
    {
        json[name] = {
            {"x", t.x},
            {"y", t.y},
            {"z", t.z},
            {"w", t.w}
        };
    }

    void WriteType(nlohmann::json& json, const std::string_view name, const glm::quat& t)
    {
        json[name] = {
          {"x", t.x},
          {"y", t.y},
          {"z", t.z},
          {"w", t.w}
        };
    }

    void ReadType(const nlohmann::json& json, const std::string_view name, glm::vec3& t)
    {
        t.x = json.at(name).at("x").get<float>();
        t.y = json.at(name).at("y").get<float>();
        t.z = json.at(name).at("z").get<float>();
    }

    void ReadType(const nlohmann::json& json, const std::string_view name, glm::vec4& t)
    {
        t.x = json.at(name).at("x").get<float>();
        t.y = json.at(name).at("y").get<float>();
        t.z = json.at(name).at("z").get<float>();
        t.w = json.at(name).at("w").get<float>();
    }

    void ReadType(const nlohmann::json& json, const std::string_view name, glm::quat& t)
    {
        t.x = json.at(name).at("x").get<float>();
        t.y = json.at(name).at("y").get<float>();
        t.z = json.at(name).at("z").get<float>();
        t.w = json.at(name).at("w").get<float>();
    }
}
