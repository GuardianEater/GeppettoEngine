/*****************************************************************//**
 * \file   SerializationSystem.cpp
 * \brief  system that saves and loads the state of the engine
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"

#include "SerializationSystem.hpp"

#include "EngineManager.hpp"

#include "Logger.hpp"

namespace Client
{
    void SerializationSystem::Initialize()
    {
        std::ifstream file("scene.json");

        if (!file.is_open())
        {
            Gep::Log::Error("Failed to open scene.json");
            return;
        }

        nlohmann::json sceneJson;
        file >> sceneJson;
        LoadScene(sceneJson);
    }

    void SerializationSystem::Exit()
    {
        const std::vector<Gep::Entity>& entities = mManager.GetEntities();

        std::ofstream file("scene.json");
        nlohmann::json sceneJson = SaveScene();
        file << sceneJson.dump();
    }

    nlohmann::json SerializationSystem::SaveEntity(Gep::Entity entity) const
    {
        nlohmann::json entityJson     = nlohmann::json::object();
        nlohmann::json componentsJson = nlohmann::json::array();
        nlohmann::json childrenJson   = nlohmann::json::array();

        mManager.ForEachComponent(entity, [&](const Gep::ComponentData& componentData)
        {
            nlohmann::json componentJson = mSaveComponentFunctions[componentData.index](entity);

            componentsJson.push_back(componentJson);
        });

        auto children = mManager.GetChildren(entity);
        for (Gep::Entity child : children)
        {
            nlohmann::json childJson = SaveEntity(child);
            childrenJson.push_back(childJson);
        }

        entityJson["children"] = childrenJson;
        entityJson["components"] = componentsJson;
        entityJson["id"] = entity;

        return entityJson;
    }

    Gep::Entity SerializationSystem::LoadEntity(const nlohmann::json& entityJson) const
    {
        Gep::Entity entity = mManager.CreateEntity();

        if (!entityJson.contains("components"))
        {
            Gep::Log::Error("Entity json does not contain components");
        }
        else
        {
            const nlohmann::json& componentsJson = entityJson["components"];

            for (const nlohmann::json& componentJson : componentsJson)
            {
                const std::string componentName        = componentJson["type"].get<std::string>();
                const nlohmann::json& componentDataJson = componentJson["data"];

                mLoadComponentFunctions.at(componentName)(entity, componentDataJson);
            }
        }

        if (!entityJson.contains("children"))
        {
            Gep::Log::Error("Entity json does not contain children");
        }
        else
        {
            nlohmann::json childrenJson = entityJson["children"];

            for (nlohmann::json childJson : childrenJson)
            {
                Gep::Entity child = LoadEntity(childJson);
                mManager.AttachEntity(entity, child);
            }
        }

        // will be used later to map old ids to new ids, in the case where a component references another entity
        if (!entityJson.contains("id"))
        {
            Gep::Log::Error("Entity json does not contain an id");
        }

        return entity;
    }

    nlohmann::json SerializationSystem::SaveScene() const
    {
        nlohmann::json sceneJson = nlohmann::json::object();
        nlohmann::json sceneData = nlohmann::json::object();
        nlohmann::json entitiesJson = nlohmann::json::array();

        std::vector<Gep::Entity> entities = mManager.GetRootEntities();
        for (Gep::Entity entity : entities)
        {
            nlohmann::json entityJson = SaveEntity(entity);
            entitiesJson.push_back(entityJson);
        }

        sceneJson["entities"] = entitiesJson;
        sceneData["entityCount"] = entities.size();
        sceneJson["sceneData"] = sceneData;

        return sceneJson;
    }

    void SerializationSystem::LoadScene(const nlohmann::json& sceneJson) const
    {
        if (!sceneJson.contains("entities"))
        {
            Gep::Log::Error("Scene json does not contain entities");
        }
        else
        {
            const nlohmann::json& entitiesJson = sceneJson["entities"];
            for (const nlohmann::json& entityJson : entitiesJson)
            {
                Gep::Entity entity = LoadEntity(entityJson);
            }
        }
    }

    template <>
    void SerializationSystem::WriteType<glm::vec3>(nlohmann::json& json, const std::string_view name, glm::vec3& t)
    {
        json[name] = { 
            {"x", t.x}, 
            {"y", t.y}, 
            {"z", t.z} 
        };
    }

    template <>
    void SerializationSystem::WriteType<glm::vec4>(nlohmann::json& json, const std::string_view name, glm::vec4& t)
    {
        json[name] = { 
            {"x", t.x}, 
            {"y", t.y}, 
            {"z", t.z}, 
            {"w", t.w} 
        };
    }

    template<>
    void SerializationSystem::WriteType(nlohmann::json& json, const std::string_view name, glm::quat& t)
    {
        json[name] = {
          {"x", t.x},
          {"y", t.y},
          {"z", t.z},
          {"w", t.w}
        };
    }

    void SerializationSystem::ReadType(const nlohmann::json& json, const std::string_view name, glm::vec3& t)
    {
        t.x = json.at(name).at("x").get<float>();
        t.y = json.at(name).at("y").get<float>();
        t.z = json.at(name).at("z").get<float>();
    }

    void SerializationSystem::ReadType(const nlohmann::json& json, const std::string_view name, glm::vec4& t)
    {
        t.x = json.at(name).at("x").get<float>();
        t.y = json.at(name).at("y").get<float>();
        t.z = json.at(name).at("z").get<float>();
        t.w = json.at(name).at("w").get<float>();
    }

    void SerializationSystem::ReadType(const nlohmann::json& json, const std::string_view name, glm::quat& t)
    {
        t.x = json.at(name).at("x").get<float>();
        t.y = json.at(name).at("y").get<float>();
        t.z = json.at(name).at("z").get<float>();
        t.w = json.at(name).at("w").get<float>();
    }
}
