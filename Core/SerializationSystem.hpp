/*****************************************************************//**
 * \file   SerializationSystem.hpp
 * \brief  Saves and loads the state of the engine
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include "ISystem.hpp"
#include "TypeID.hpp"
#include "TypeList.hpp"
#include "EngineManager.hpp"
#include "Logger.hpp"

#include <fstream>

#include "nlohmann/json.hpp"

namespace Client
{

    template <typename T>
    concept JsonWritable = requires(const T t, nlohmann::json& json)
    {
        json = t;
    };

    template <typename T>
    concept JsonReadable = requires(T t, const nlohmann::json& json)
    {
        t = json.get<T>();
    };

    class SerializationSystem : public Gep::ISystem
    {
    private:

    public:
        SerializationSystem(Gep::EngineManager& em)
            : ISystem(em)
        {}

        template <typename... ComponentTypes>
        void OnComponentsRegistered(Gep::type_list<ComponentTypes...> componentTypes);

        void Initialize() override;
        void Exit() override;

        nlohmann::json SaveEntity(Gep::Entity entity) const;
        Gep::Entity LoadEntity(const nlohmann::json& entityJson) const;

        nlohmann::json SaveScene() const;
        void LoadScene(const nlohmann::json& sceneJson) const;

    private:
        // base case, do nothing
        template <typename T>
        void WriteType(nlohmann::json& json, const std::string_view name, T& t);

        // default case, just write the value
        template <typename T>
        requires JsonWritable<T>
        void WriteType(nlohmann::json& json, const std::string_view name, T& t);

        // bunch of base type specializations
        template <> void WriteType<glm::vec3>(nlohmann::json& json, const std::string_view name, glm::vec3& t);
        template <> void WriteType<glm::vec4>(nlohmann::json& json, const std::string_view name, glm::vec4& t);

        void ReadType(const nlohmann::json& json, const std::string_view name, glm::vec3& t);
        void ReadType(const nlohmann::json& json, const std::string_view name, glm::vec4& t);

        // base case, do nothing
        template <typename T>
        void ReadType(const nlohmann::json& json, const std::string_view name, T& t);

        // default case, just read the value
        template <typename T>
        requires JsonReadable<T>
        void ReadType(const nlohmann::json& json, const std::string_view name, T& t);

        // bunch of base type specializations
    private:

        // component index -> function that converts component to json
        std::vector<std::function<nlohmann::json(Gep::Entity)>> mSaveComponentFunctions;
        std::map<std::string, std::function<void(Gep::Entity, const nlohmann::json&)>> mLoadComponentFunctions;
    };
}

#include "SerializationSystem.inl"