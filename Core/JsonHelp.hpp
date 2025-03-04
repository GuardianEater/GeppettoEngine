/*****************************************************************//**
 * \file   JsonHelp.hpp
 * \brief  helper functions to work with nlohmann to serialize and deserialize types
 *
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

namespace Gep
{
    namespace Json
    {
        template <typename T>
        concept JsonWritable = requires(const T t, nlohmann::json & json)
        {
            json = t;
        };

        template <typename T>
        concept JsonReadable = requires(T t, const nlohmann::json & json)
        {
            t = json.get<T>();
        };

        template<typename T>
        void WriteType(nlohmann::json& json, const std::string_view name, T& t)
        {
            json[name] = 0;
        }

        template <typename T>
            requires JsonWritable<T>
        void WriteType(nlohmann::json& json, const std::string_view name, T& t)
        {
            json[name] = t;
        }

        template<typename T>
        void ReadType(const nlohmann::json& json, const std::string_view name, T& t)
        {
            // do nothing if the type is not readable
        }

        template <typename T>
            requires JsonReadable<T>
        void ReadType(const nlohmann::json& json, const std::string_view name, T& t)
        {
            t = json[name].get<std::remove_reference_t<T>>();
        }

        // bunch of base type specializations
        void WriteType(nlohmann::json& json, const std::string_view name, const glm::vec3& t);
        void WriteType(nlohmann::json& json, const std::string_view name, const glm::vec4& t);
        void WriteType(nlohmann::json& json, const std::string_view name, const glm::quat& t);

        void ReadType(const nlohmann::json& json, const std::string_view name, glm::vec3& t);
        void ReadType(const nlohmann::json& json, const std::string_view name, glm::vec4& t);
        void ReadType(const nlohmann::json& json, const std::string_view name, glm::quat& t);

    }
}