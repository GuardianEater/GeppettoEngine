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
#include <PhysicsSystem.hpp>
#include <ImGuiSystem.hpp>
#include <WindowSystem.hpp>
#include <RenderSystem.hpp>
#include <ScriptingSystem.hpp>
#include <CameraComponent.hpp>

#include "Logger.hpp"

#include <rfl.hpp>
#include <rfl/json.hpp>

// tools
#include <CompactArray.hpp>

int main()
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
        Client::Camera
    > componentTypes;

    // list of all systems //////////////////////////////////////////////////////////////////////////
    Gep::type_list<
        Client::PhysicsSystem,
        Client::WindowSystem,
        Client::RenderSystem,
        Client::ImGuiSystem,
        Client::ScriptingSystem
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
    {
        const float aspect = 1;
        const float nearPlane = 0.1;
        const float farPlane = 1000;
        const float fov = 80;

        const glm::vec3 lookAt = -glm::vec3(0, 0, 1);
        const glm::vec3 relativeUp = glm::vec3(0, 1, 0);
        const glm::vec3 back = -glm::normalize(lookAt);
        const glm::vec3 right = glm::normalize(glm::cross(lookAt, relativeUp));

        glm::vec3 viewport{};
        viewport.x = (2.0f) * (nearPlane)*tanf(glm::radians(fov / 2.0f));
        viewport.y = viewport.x / aspect;
        viewport.z = nearPlane;

        em.AddComponent(camera,
            Client::Transform
            {
              .position = glm::vec3(0, 0, 10),
              .scale = glm::vec3(5, 5, 5),
              .rotation = glm::vec3(0, 0, 0),
            },
            Client::Camera{},
            Client::Identification
            {
              .name = "Camera"
            });
    }

    // more entites
    for (int i = 0; i < 3; i++)
    {
        Gep::Entity entity = em.CreateEntity();
        {
            em.AddComponent(entity,
                Client::Transform
                {
                  .position = glm::vec3(0, 0, 0),
                  .scale = glm::vec3(5, 5, 5),
                  .rotation = glm::vec3(0, 0, 0),
                },
                Client::RigidBody
                {
                  .velocity = {0, 0, 0},
                  .acceleration = {0, 0, 0},
                  .rotationalVelocity = {0, 0, 0},
                  .rotationalAcceleration = {0, 0, 0}
                },
                Client::Material
                {
                  .diff_coeff = { 0.5, 1, 0.5 },
                  .spec_coeff = { 0.5, 0.5, 0.5 },
                  .spec_exponent = 5,
                  .meshID = 0
                },
                Client::Identification
                {
                  .name = "Sphere"
                });
        }
    }

    Gep::Entity e1 = em.CreateEntity();
    {
        em.AddComponent(e1,
            Client::Transform{},
            Client::Material{},
            Client::Identification
            {
              .name = "SphereParent"
            });
    }

    Gep::Entity e2 = em.CreateEntity();
    {
        em.AddComponent(e2,
            Client::Transform{},
            Client::Material{},
            Client::Identification
            {
              .name = "SphereChild1"
            });
    }

    Gep::Entity e3 = em.CreateEntity();
    {
        em.AddComponent(e3,
            Client::Transform{},
            Client::Material{},
            Client::Identification
            {
              .name = "SphereChild2"
            });
    }

    em.AttachEntity(e1, e2);
    em.AttachEntity(e1, e3);
    

    double dt = 0.016;
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
