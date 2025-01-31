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

// components
#include "CameraComponent.hpp"
#include "ActiveCameraComponent.hpp"
#include "Identification.hpp"
#include "Material.hpp"
#include "RigidBody.hpp"
#include "Script.hpp"
#include "Transform.hpp"
#include "TextureComponent.hpp"

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
    em.Start();

    // list of all components ///////////////////////////////////////////////////////////////////////
    Gep::type_list<
        Client::Identification,
        Client::Transform,
        Client::RigidBody,
        Client::Material,
        Client::Script,
        Client::ActiveCamera,
        Client::Camera,
        Client::Texture
    > componentTypes;

    // list of all systems //////////////////////////////////////////////////////////////////////////
    Gep::type_list<
        Client::PhysicsSystem,
        Client::WindowSystem,
        Client::RenderSystem,
        Client::ImGuiSystem,
        Client::ScriptingSystem,
        Client::SerializationSystem
    > systemTypes;

    // register all types ////////////////////////////////////////////////////////////////////////////
    em.RegisterTypes(componentTypes, systemTypes);

    // setup entity groups //////////////////////////////////////////////////////////////////////////
    em.RegisterGroup<Client::RigidBody, Client::Transform>();
    em.RegisterGroup<Client::Material, Client::Transform>();
    em.RegisterGroup<Client::Camera, Client::Transform>();
    em.RegisterGroup<Client::Script>();
    em.RegisterGroup(); // empty group with all entities

    // subscribe to events
    em.SubscribeToEvent<Client::PhysicsSystem, Gep::Event::EntityDestroyed>(&Client::PhysicsSystem::EntityDestroyed);
    em.SubscribeToEvent<Client::PhysicsSystem, Gep::Event::KeyPressed>(&Client::PhysicsSystem::KeyPressed);
    em.SubscribeToEvent<Client::RenderSystem, Gep::Event::KeyPressed>(&Client::RenderSystem::KeyEvent);


    // initialize systems ////////////////////////////////////////////////////////////////////////////
    em.Initialize();

    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// ECS testing
    ///////////////////////////////////////////////////////////////////////////////////////////////

    Gep::Entity camera = em.CreateEntity();
    em.AddComponent(camera,
        Client::Transform{ {0, 0, 7} },
        Client::Camera{},
        Client::Identification{ "Camera" }
    );

    Gep::Entity floor = em.CreateEntity();
    em.AddComponent(floor,
        Client::Transform{ {0, -3, 0},  {10, 1, 10} },
        Client::Material{ "Cube" },
        Client::Texture{ "Checker" },
        Client::Identification{ "Floor" }
    );

    Gep::Entity entity = em.CreateEntity();
    em.AddComponent(entity,
        Client::Transform{ {-3.0f, 3.0f, -3.0f} },
        Client::RigidBody{},
        Client::Material{ "Icosphere" },
        Client::Identification{ "Sphere" }
    );

    Gep::Entity e1 = em.CreateEntity();
    em.AddComponent(e1,
        Client::Transform{ {-4, -1, -7}, {1, 1, 1}, {1, 16, 1} },
        Client::Material{ "Cube" },
        Client::Texture{ "Fox" },
        Client::Identification{ "Fox Box" }
    );

    Gep::Entity e2 = em.CreateEntity();
    em.AddComponent(e2,
        Client::Transform{ {-8, -1, -4}, {1,1,1}, {0, 80, 0} },
        Client::Material{ "Cube" },
        Client::Texture{ "Raccoon" },
        Client::Identification{ "Raccube" }
    );

    Gep::Entity e4 = em.CreateEntity();
    em.AddComponent(e4,
        Client::Transform{ {-7, 0, -7}, {1, 2, 1}, {0, 45, 0} },
        Client::Material{ "Cube" },
        Client::Texture{ "Kurisu" },
        Client::Identification{ "Kurisu" }
    );

    Gep::Entity e5 = em.CreateEntity();
    em.AddComponent(e5,
        Client::Transform{ { 6, 1, -5 }, { 1, 1, 1 }, { 0, -23, 0 } },
        Client::Material{ "Cube" },
        Client::Texture{ "Okayu1" },
        Client::Identification{ "Okayu1" }
    );

    Gep::Entity e6 = em.CreateEntity();
    em.AddComponent(e6,
        Client::Transform{ { 6, -1, -5 }, { 2, 1, 2 }, { 0, -45, 0 } },
        Client::Material{ "Cube" },
        Client::Texture{ "Okayu2" },
        Client::Identification{ "Okayu2" }
    );

    float dt = 0.016f;
    while (em.Running())
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        em.FrameStart();

        // update systems /////////////////////////////////////////////////////////////////////////
        em.Update(dt);

        // start events ///////////////////////////////////////////////////////////////////////////
        em.StartEvent<Gep::Event::EntityDestroyed>();
        em.StartEvent<Gep::Event::KeyPressed>();

        // TODO: make this a ResolveEvents call, or perhaps add that to FrameEnd
        em.DestroyMarkedComponents();
        em.DestroyMarkedEntities();

        em.FrameEnd();
        auto endTime = std::chrono::high_resolution_clock::now();
        dt = std::chrono::duration<float, std::chrono::seconds::period>(endTime - startTime).count();
    }

    em.Exit();
    em.End();
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
