/*****************************************************************//**
 * \file   main.cpp
 * \brief  entry point for the engine
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

 // core
#include <Core.hpp>
#include <EngineManager.hpp>

// rendering
#include <Renderer.hpp>
#include <SphereMesh.hpp>
#include <Affine.hpp>

// client
#include "PhysicsSystem.hpp"
#include "ImGuiSystem.hpp"
#include "WindowSystem.hpp"
#include "RenderSystem.hpp"
#include "ScriptingSystem.hpp"
#include "SerializationSystem.hpp"

#include <CameraComponent.hpp>

#include "Logger.hpp"

#include <rfl.hpp>
#include <rfl/json.hpp>

// tools
#include <CompactArray.hpp>

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

    Gep::Entity entity = em.CreateEntity();
    em.AddComponent(entity,
        Client::Transform{ {-3.0f, 3.0f, -3.0f} },
        Client::RigidBody{},
        Client::Material{ "Sphere" },
        Client::Identification{ "Sphere" }
    );

    Gep::Entity e1 = em.CreateEntity();
    em.AddComponent(e1,
        Client::Transform{},
        Client::Material{ "Cube" },
        Client::Texture{ "test" },
        Client::Identification{ "CubeParent" }
    );
    

    Gep::Entity e2 = em.CreateEntity();
    em.AddComponent(e2,
        Client::Transform{},
        Client::Material{ "Cube" },
        Client::Identification{ "CubeChild1" }
    );
    

    Gep::Entity e3 = em.CreateEntity();
    em.AddComponent(e3,
        Client::Transform{},
        Client::Material{ "Cube" },
        Client::Identification{ "CubeChild2" }
    );
    

    em.AttachEntity(e1, e2);
    em.AttachEntity(e1, e3);


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
    return 1;
}
catch (...)
{
    return 1;
}
