/*****************************************************************//**
 * \file   main.cpp
 * \brief  entry point for the engine
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "pch.hpp"

// engine
#include "Core.hpp"
#include "EngineManager.hpp"
#include "Logger.hpp"

// resources
#include "Renderer.hpp"
#include "ScriptingResource.hpp"
#include "SoundResource.hpp"
#include "CollisionResource.hpp"
#include "SerializationResource.hpp"
#include "EditorResource.hpp"

// components
#include "CameraComponent.hpp"
#include "ActiveCameraComponent.hpp"
#include "Identification.hpp"
#include "Material.hpp"
#include "RigidBody.hpp"
#include "Script.hpp"
#include "Transform.hpp"
#include "TextureComponent.hpp"
#include "LightComponent.hpp"
#include "SoundComponent.hpp"
#include "SphereCollider.hpp"
#include "CubeCollider.hpp"

 // systems
#include "PhysicsSystem.hpp"
#include "ImGuiSystem.hpp"
#include "WindowSystem.hpp"
#include "RenderSystem.hpp"
#include "ScriptingSystem.hpp"
#include "SerializationSystem.hpp"
#include "RelationSystem.hpp"
#include "SoundSystem.hpp"
#include "CollisionSystem.hpp"

#define SOLLOUD_DYNAMIC
#include "soloud.h"
#include "soloud_wav.h"

int main() try
{
    Gep::Log::SetPrintLevel(Gep::Log::LogLevel::info);
    Gep::Log::SetOutputFile("log.txt");

    Gep::Log::Important("Welcome To The Gep Engine!");

    // start the engine //////////////////////////////////////////////////////////////////////////////
    Gep::EngineManager em;

    // register all resources ////////////////////////////////////////////////////////////////////////
    em.RegisterResource<Client::ScriptingResource>();
    em.RegisterResource<Client::SoundResource>();
    em.RegisterResource<Client::CollisionResource>();
    em.RegisterResource<Client::SerializationResource>();
    em.RegisterResource<Client::EditorResource>();

    // list of all components ///////////////////////////////////////////////////////////////////////
    Gep::type_list<
        Client::Identification,
        Client::Transform,
        Client::RigidBody,
        Client::Mesh,
        Client::Script,
        Client::ActiveCamera,
        Client::Camera,
        Client::Texture,
        Client::Light,
        Client::SpatialSoundEmitter,
        Client::SphereCollider,
        Client::CubeCollider
    > componentTypes;

    // list of all systems //////////////////////////////////////////////////////////////////////////
    Gep::type_list<
        Client::WindowSystem,
        Client::ImGuiSystem,
        Client::RenderSystem,
        Client::ScriptingSystem,
        Client::PhysicsSystem,
        Client::SerializationSystem,
        Client::RelationSystem,
        Client::SoundSystem,
        Client::CollisionSystem
    > systemTypes;

    // register all types ////////////////////////////////////////////////////////////////////////////
    em.RegisterTypes(componentTypes, systemTypes);

    em.RegisterResource<Gep::OpenGLRenderer>();

    // initialize systems ////////////////////////////////////////////////////////////////////////////
    em.Initialize();

    while (em.Running())
    {
        em.FrameStart();
        em.Update();
        em.FrameEnd();

        em.DestroyMarkedComponents();
        em.DestroyMarkedEntities();
    }

    em.Exit();
}
catch (const std::exception& e)
{
    Gep::Log::Error("Caught exception: ", e.what());
    return 1;
}
catch (...)
{
    return 1;
}
