/*****************************************************************//**
 * \file   SerializationSystem.cpp
 * \brief  system that saves and loads the state of the engine
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"

#include "SerializationSystem.hpp"
#include "SerializationResource.hpp"

#include "EngineManager.hpp"

#include "Logger.hpp"

namespace Client
{
    void SerializationSystem::Initialize()
    {
        mManager.GetResource<SerializationResource>().LoadScene(mManager, "assets\\scenes\\default.scenejson");
    }

    void SerializationSystem::Exit()
    {
        mManager.GetResource<SerializationResource>().SaveScene(mManager);
    }
}
