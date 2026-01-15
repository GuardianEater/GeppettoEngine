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

// components
#include "CameraComponent.hpp"
#include "ActiveCameraComponent.hpp"
#include "ModelComponent.hpp"
#include "RigidBody.hpp"
#include "Script.hpp"
#include "Transform.hpp"
#include "TextureComponent.hpp"
#include "LightComponent.hpp"
#include "SoundComponent.hpp"
#include "SphereCollider.hpp"
#include "CubeCollider.hpp"
#include "AnimationComponent.hpp"
#include "MeshCollider.hpp"
#include "CurveComponent.hpp"
#include "PathFollowerComponent.hpp"
#include "IKTarget.hpp"

// resources
#include "ScriptingResource.hpp"
#include "SoundResource.hpp"
#include "CollisionResource.hpp"
#include "SerializationResource.hpp"
#include "EditorResource.hpp"
#include "OpenGLRenderer.hpp"

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
#include "AnimationSystem.hpp"
#include "CurveSystem.hpp"
#include "IKSystem.hpp"

#include "OS.hpp"

int main()  
{
    Gep::SetDynamicLibraryDirectory("lib");

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
    em.RegisterResource<Gep::OpenGLRenderer>();

    // list of all components ///////////////////////////////////////////////////////////////////////
    Gep::TypeList <
        Client::Transform,
        Client::RigidBody,
        Client::Spring,
        Client::RiggedModelComponent,
        Client::StaticModelComponent,
        //Client::Script,
        //Client::GameCamera,
        Client::Camera,
        //Client::Texture,
        Client::Light,
        Client::DirectionalLight,
        Client::SpatialSoundEmitter,
        Client::SphereCollider,
        Client::CubeCollider,
        Client::AnimationComponent,
        Client::MeshCollider,
        Client::CurveComponent,
        Client::PathFollowerComponent,
        Client::IKTarget
    > componentTypes;

    // list of all systems //////////////////////////////////////////////////////////////////////////
    Gep::TypeList<
        Client::WindowSystem,
        Client::ImGuiSystem,
        Client::AnimationSystem, // must happen before the render system
        Client::IKSystem,
        Client::CurveSystem,
        Client::RenderSystem,
        //Client::ScriptingSystem,
        Client::SerializationSystem,
        Client::PhysicsSystem,
        Client::RelationSystem,
        Client::SoundSystem,
        Client::CollisionSystem
    > systemTypes;

    // register all types ////////////////////////////////////////////////////////////////////////////
    em.RegisterTypes(componentTypes, systemTypes);

    //em.SetSystemExecutionPolicy<Client::ScriptingSystem>(Gep::EngineState::Play);

    // initialize systems ////////////////////////////////////////////////////////////////////////////
    em.SetState(Gep::EngineState::Edit);
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
