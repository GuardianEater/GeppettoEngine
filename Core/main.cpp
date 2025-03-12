/*****************************************************************//**
 * \file   main.cpp
 * \brief  entry point for the engine
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "pch.hpp"

 // systems
#include "PhysicsSystem.hpp"
#include "ImGuiSystem.hpp"
#include "WindowSystem.hpp"
#include "RenderSystem.hpp"
#include "ScriptingSystem.hpp"
#include "SerializationSystem.hpp"
#include "RelationSystem.hpp"

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

// engine
#include "Core.hpp"
#include "EngineManager.hpp"
#include "Logger.hpp"

int main() try
{
    Gep::Log::SetPrintLevel(Gep::Log::LogLevel::info);
    Gep::Log::SetOutputFile("log.txt");

    Gep::Log::Important("Welcome To The Gep Engine!");

    // start the engine //////////////////////////////////////////////////////////////////////////////
    Gep::EngineManager em;

    // register all resources ////////////////////////////////////////////////////////////////////////
    em.RegisterResource<Client::ScriptingResource>();

    // list of all components ///////////////////////////////////////////////////////////////////////
    Gep::type_list<
        Client::Identification,
        Client::Transform,
        Client::RigidBody,
        Client::Material,
        Client::Script,
        Client::ActiveCamera,
        Client::Camera,
        Client::Texture,
        Client::Light
    > componentTypes;

    // list of all systems //////////////////////////////////////////////////////////////////////////
    Gep::type_list<
        Client::WindowSystem,
        Client::ImGuiSystem,
        Client::RenderSystem,
        Client::ScriptingSystem,
        Client::PhysicsSystem,
        Client::SerializationSystem,
        Client::RelationSystem
    > systemTypes;


    // register all types ////////////////////////////////////////////////////////////////////////////
    em.RegisterTypes(componentTypes, systemTypes);

    em.RegisterResource<Gep::OpenGLRenderer>();

    // setup entity groups //////////////////////////////////////////////////////////////////////////
    em.RegisterGroup<Client::RigidBody, Client::Transform>();
    em.RegisterGroup<Client::Material, Client::Transform>();
    em.RegisterGroup<Client::Camera, Client::Transform>();
    em.RegisterGroup<Client::Light, Client::Transform>();
    em.RegisterGroup<Client::Transform>();
    em.RegisterGroup<Client::Script>();
    em.RegisterGroup(); // empty group with all entities

    // initialize systems ////////////////////////////////////////////////////////////////////////////
    em.Initialize();
    em.ResolveEvents();

    while (em.Running())
    {
        em.FrameStart();
        em.Update();
        em.FrameEnd();

        em.DestroyMarkedComponents();
        em.DestroyMarkedEntities();
        em.ResolveEvents();

        Gep::Log::Critical("Frame End");
    }

    em.Exit();
    em.ResolveEvents();
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
