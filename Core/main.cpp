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
#include "ModelComponent.hpp"
#include "RigidBody.hpp"
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
    // initialize logging ////////////////////////////////////////////////////////////////////////////
    Gep::Log::SetPrintLevel(Gep::Log::LogLevel::Info);
    Gep::Log::SetOutputFile("log.txt");
    Gep::Log::Important("Welcome To The Gep Engine!");

    // start the engine //////////////////////////////////////////////////////////////////////////////
    Gep::EngineManager em;

    // register all resources ////////////////////////////////////////////////////////////////////////
    gtl::type_list<
        //Client::ScriptingResource,
        Client::SoundResource,
        Client::CollisionResource,
        Client::SerializationResource,
        Client::EditorResource,
        Gep::OpenGLRenderer
    > resourceTypes;

    // list of all components ///////////////////////////////////////////////////////////////////////
    gtl::type_list<
        Client::Transform,
        Client::RigidBody,
        Client::Spring,
        Client::RiggedModelComponent,
        Client::StaticModelComponent,
        Client::Camera,
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
    gtl::type_list<
        Client::WindowSystem,
        Client::AnimationSystem, // must happen before the render system
        Client::IKSystem,
        Client::CurveSystem,
        Client::RenderSystem, // must happen before the imgui system
        Client::ImGuiSystem,
        Client::SerializationSystem,
        Client::PhysicsSystem,
        Client::RelationSystem,
        Client::SoundSystem,
        Client::CollisionSystem
    > systemTypes;

    // register all types ////////////////////////////////////////////////////////////////////////////
    em.RegisterTypes(resourceTypes, componentTypes, systemTypes);

    // initialize systems ////////////////////////////////////////////////////////////////////////////
    em.SetState(Gep::EngineState::Edit);
    em.Initialize();

    while (em.IsRunning())
    {
        em.FrameStart();
        em.Update();
        em.FrameEnd();

        em.DestroyMarkedComponents();
        em.DestroyMarkedEntities();
    }

    em.Exit();
}
