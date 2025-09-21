/*****************************************************************//**
 * \file   SerializationSystem.cpp
 * \brief  system that saves and loads the state of the engine
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"

#include "SerializationSystem.hpp"
#include "SerializationResource.hpp"
#include "EditorResource.hpp"

#include "EngineManager.hpp"

#include "Logger.hpp"

namespace Client
{
    void SerializationSystem::OnAssetBrowserItemClicked(const Gep::Event::AssetBrowserItemClicked& event)
    {
        if (event.extension == ".scene")
        {
            mManager.GetResource<Client::SerializationResource>().ChangeScene(mManager, event.path);
        }
        else if (event.extension == ".prefab")
        {
            nlohmann::json prefab = mManager.GetResource<Client::SerializationResource>().LoadPrefab(event.path);
            Gep::Entity prefabEntity = mManager.LoadEntity(prefab);
            mManager.SetUUID(prefabEntity, Gep::UUID::GenerateNew());
            mManager.GetResource<Client::EditorResource>().SelectEntity(prefabEntity);
        }
    }
    void SerializationSystem::OnEngineStateChanged(const Gep::Event::StateChanged& event)
    {
        // if the new state is not game or paused
        if (!static_cast<uint8_t>(event.newState & Gep::EngineState::Game) && !static_cast<uint8_t>(event.newState & Gep::EngineState::Paused))
        {
            mManager.GetResource<Client::SerializationResource>().ReloadScene(mManager);
        }
    }
    void SerializationSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::AssetBrowserItemClicked>(this, &SerializationSystem::OnAssetBrowserItemClicked);
        mManager.SubscribeToEvent<Gep::Event::StateChanged>(this, &SerializationSystem::OnEngineStateChanged);

        mManager.GetResource<SerializationResource>().LoadScene(mManager, "assets\\scenes\\default.scene");
    }

    void SerializationSystem::Exit()
    {
        mManager.GetResource<SerializationResource>().SaveScene(mManager);
    }
}
