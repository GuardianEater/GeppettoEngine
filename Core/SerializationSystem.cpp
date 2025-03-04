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
    std::filesystem::path scenePath = "assets\\scenes\\scene.json";
    void SerializationSystem::Initialize()
    {
        std::ifstream file(scenePath);

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

        std::ofstream file(scenePath);
        nlohmann::json sceneJson = SaveScene();
        file << sceneJson.dump();
    }

    nlohmann::json SerializationSystem::SaveScene() const
    {
        nlohmann::json sceneJson = nlohmann::json::object();
        nlohmann::json sceneData = nlohmann::json::object();
        nlohmann::json entitiesJson = nlohmann::json::array();

        std::vector<Gep::Entity> entities = mManager.GetRootEntities();
        for (Gep::Entity entity : entities)
        {
            nlohmann::json entityJson = mManager.SaveEntity(entity);
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
            return;
        }

        const nlohmann::json& entitiesJson = sceneJson["entities"];
        for (const nlohmann::json& entityJson : entitiesJson)
        {
            Gep::Entity entity = mManager.LoadEntity(entityJson);
        }
    }
}
