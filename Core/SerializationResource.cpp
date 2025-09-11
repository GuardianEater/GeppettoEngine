/*****************************************************************//**
 * \file   SerializationResource.cpp
 * \brief  Implementation of SerializationResource
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
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

    void SerializationResource::NewScene(const std::filesystem::path& path) const
    {
        std::ofstream outFile(path);

        nlohmann::json sceneJson = nlohmann::json::object();

        sceneJson["entities"] = nlohmann::json::array();
        sceneJson["sceneData"] = nlohmann::json::object();

        outFile << sceneJson.dump();
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

    void SerializationResource::SavePrefab(const nlohmann::json& entityJson, const std::filesystem::path& path) const
    {
        if (!std::filesystem::exists(path))
            std::filesystem::create_directories(path.parent_path());

        nlohmann::json prefab = entityJson;
        if (prefab.contains("uuid"))
            prefab["uuid"] = Gep::UUID{}.ToString(); // assign an invalid uuid so a new one will get generated on load

        std::ofstream outFile(path);
        outFile << prefab.dump();
    }

    nlohmann::json SerializationResource::LoadPrefab(const std::filesystem::path& path) const
    {
        if (!std::filesystem::exists(path))
        {
            Gep::Log::Error("Failed to load prefab: [", path.string(), "] doesn't exist");
            return nlohmann::json();
        }

        nlohmann::json prefab;
        std::ifstream inFile(path);
        inFile >> prefab;

        return prefab;
    }

    nlohmann::json SerializationResource::SerializeScene(Gep::EngineManager& em) const
    {
        nlohmann::json sceneJson = nlohmann::json::object();
        nlohmann::json sceneData = nlohmann::json::object();
        nlohmann::json entitiesJson = nlohmann::json::array();

        std::vector<Gep::Entity> entities = em.GetRoots();
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
