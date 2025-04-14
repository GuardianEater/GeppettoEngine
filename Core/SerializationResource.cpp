/*****************************************************************//**
 * \file   SerializationResource.cpp
 * \brief  Implementation of SerializationResource
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#include "pch.hpp"
#include "SerializationResource.hpp"
#include "EngineManager.hpp"

namespace Client
{
    void SerializationResource::SaveScene(Gep::EngineManager& em, const std::filesystem::path& path) const
    {
        Gep::Log::Trace("Saving scene to: ", path.string());

        if (!std::filesystem::exists(path.parent_path()))
        {
            Gep::Log::Error("Scene path does not exist: ", path.parent_path().string());
            return;
        }

        nlohmann::json sceneJson = SerializeScene(em);
        std::ofstream file(path);
        if (!file.is_open())
        {
            Gep::Log::Error("Failed to save current scene as [", path.string(), "]");
            return;
        }
        file << sceneJson.dump();

        Gep::Log::Info("Scene saved to: ", path.string());
    }

    void SerializationResource::LoadScene(Gep::EngineManager& em, const std::filesystem::path& path) const
    {
        Gep::Log::Trace("Loading scene from: ", path.string());

        if (!std::filesystem::exists(path))
        {
            Gep::Log::Error("Scene file does not exist: ", path.string());
            return;
        }

        std::ifstream file(path);

        if (!file.is_open())
        {
            Gep::Log::Error("Failed to open: ", path.string());
            return;
        }

        nlohmann::json sceneJson;
        file >> sceneJson;
        DeserializeScene(em, sceneJson);

        Gep::Log::Info("Scene loaded from: ", path.string());
    }

    void SerializationResource::SaveScene(Gep::EngineManager& em) const
    {
        SaveScene(em, currentScenePath);
    }

    void SerializationResource::ReloadScene(Gep::EngineManager& em)
    {
        ChangeScene(em, currentScenePath);
    }

    void SerializationResource::ChangeScene(Gep::EngineManager& em, const std::filesystem::path& path)
    {
        SaveScene(em, currentScenePath);
        currentScenePath = path;
        em.DestroyAllEntities();
        LoadScene(em, path);
    }

    nlohmann::json SerializationResource::SerializeScene(Gep::EngineManager& em) const
    {
        nlohmann::json sceneJson = nlohmann::json::object();
        nlohmann::json sceneData = nlohmann::json::object();
        nlohmann::json entitiesJson = nlohmann::json::array();

        std::vector<Gep::Entity> entities = em.GetRootEntities();
        for (Gep::Entity entity : entities)
        {
            nlohmann::json entityJson = em.SaveEntity(entity);
            entitiesJson.push_back(entityJson);
        }

        sceneJson["entities"] = entitiesJson;
        sceneData["entityCount"] = entities.size();
        sceneJson["sceneData"] = sceneData;

        return sceneJson;
    }

    void SerializationResource::DeserializeScene(Gep::EngineManager& em, const nlohmann::json& sceneJson) const
    {
        if (!sceneJson.contains("entities"))
        {
            Gep::Log::Error("Scene json does not contain entities");
            return;
        }

        const nlohmann::json& entitiesJson = sceneJson["entities"];
        for (const nlohmann::json& entityJson : entitiesJson)
        {
            Gep::Entity entity = em.LoadEntity(entityJson);
        }
    }
}
