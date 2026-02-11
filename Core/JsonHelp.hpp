/*****************************************************************//**
 * \file   JsonHelp.hpp
 * \brief  helper functions to work with nlohmann to serialize and deserialize types
 *
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <TypeID.hpp>
#include <uuid.hpp>
#include "VQS.hpp"
// adds overloads for nlohmann
namespace nlohmann
{
    void to_json(json& j, const glm::vec3& v);
    void from_json(const json& j, glm::vec3& v);
    void to_json(json& j, const glm::vec4& v);
    void from_json(const json& j, glm::vec4& v);
    void to_json(json& j, const glm::quat& v);
    void from_json(const json& j, glm::quat& v);
    void to_json(json& j, const glm::mat3& v);
    void from_json(const json& j, glm::mat3& v);
    void to_json(json& j, const glm::mat4& v);
    void from_json(const json& j, glm::mat4& v);
    void to_json(json& j, const gtl::uuid& v);
    void from_json(const json& j, gtl::uuid& v);
    void to_json(json& j, const Gep::VQS& v);
    void from_json(const json& j, Gep::VQS& v);
}

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
        void WriteType(nlohmann::json& json, T& t)
        {
            // catch all applied to all types that are not supprted by nlohmann
            // do nothing if the type is not writable

            //Gep::Log::Warning("Attempting to write an unsupported type to json. Type was: [", Gep::GetTypeInfo<T>().Name(), "]");
        }

        template <typename T>
            requires JsonWritable<T>
        void WriteType(nlohmann::json& json, T& t)
        {
            json = t;
        }

        template<typename T>
        void ReadType(const nlohmann::json& json, T& t)
        {
            // catch all applied to all types that are not supprted by nlohmann
            // do nothing if the type is not readable

            //Gep::Log::Warning("Attempting to read an unsupported type from json. Type was: [", Gep::GetTypeInfo<T>().Name(), "]");
        }

        template <typename T>
            requires JsonReadable<T>
        void ReadType(const nlohmann::json& json, T& t)
        {
            t = json.get<std::remove_reference_t<T>>();
        }
    }
}