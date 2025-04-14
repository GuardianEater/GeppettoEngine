/*****************************************************************//**
 * \file   SerializationResource.hpp
 * \brief  SerializationResource is a resource that can assists in saving and loading the state of the engine
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"

#include "nlohmann/json.hpp"

namespace Gep
{
    class EngineManager;
}

namespace Client
{
    class SerializationResource
    {
    public:
        void SaveScene(Gep::EngineManager& em, const std::filesystem::path& path) const; // writes all current entities to a file
        void LoadScene(Gep::EngineManager& em, const std::filesystem::path& path) const; // will simply load all entities from a file, it will not clear the scene nor set the active scene

        void SaveScene(Gep::EngineManager& em) const; // saves the current scene to the currentScenePath

        void ReloadScene(Gep::EngineManager& em); // reloads the current scene from the currentScenePath
        void ChangeScene(Gep::EngineManager& em, const std::filesystem::path& path); // saves the current scene, clears all entities, then loads a new scene


        nlohmann::json SerializeScene(Gep::EngineManager& em) const;
        void DeserializeScene(Gep::EngineManager& em, const nlohmann::json& sceneJson) const;

        std::filesystem::path currentScenePath = "assets/scenes/default.scenejson";
    };
}
