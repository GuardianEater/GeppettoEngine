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
            mManager.GetResource<Client::EditorResource>().SelectEntity(prefabEntity);
        }
    }
    void SerializationSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::AssetBrowserItemClicked>(this, &SerializationSystem::OnAssetBrowserItemClicked);

        mManager.GetResource<SerializationResource>().LoadScene(mManager, "assets\\scenes\\default.scene");
    }

    void SerializationSystem::Exit()
    {
        mManager.GetResource<SerializationResource>().SaveScene(mManager);
    }
}
