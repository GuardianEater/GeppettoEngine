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

// client
#include <PhysicsSystem.hpp>
#include <ImGuiSystem.hpp>
#include <WindowSystem.hpp>
#include <RenderSystem.hpp>

// tools
#include <CompactArray.hpp>

#define OUT_TYPE_HASH(type) std::cout << #type << ": " << typeid(Client::type).hash_code() << std::endl

int main()
{
	Gep::EngineManager em;

	// register all components ////////////////////////////////////////////////////////////////////
	em.RegisterComponent<Client::RigidBody>();
	em.RegisterComponent<Client::Material>();
	em.RegisterComponent<Client::Identification>();
	em.RegisterComponent<Client::Transform>();

	// register all systems ///////////////////////////////////////////////////////////////////////
	em.RegisterSystem<Client::PhysicsSystem>();
	em.RegisterSystem<Client::WindowSystem>();
	em.RegisterSystem<Client::RenderSystem>();
	em.RegisterSystem<Client::ImGuiSystem>();

	// set system signature ///////////////////////////////////////////////////////////////////////
	Gep::Signature physicsSystemSignature;
	physicsSystemSignature.set(em.GetComponentID<Client::RigidBody>());
	physicsSystemSignature.set(em.GetComponentID<Client::Transform>());
	em.SetSystemSignature<Client::PhysicsSystem>(physicsSystemSignature);

	Gep::Signature renderSystemSignature;
	renderSystemSignature.set(em.GetComponentID<Client::Material>());
	renderSystemSignature.set(em.GetComponentID<Client::Transform>());
	em.SetSystemSignature<Client::RenderSystem>(renderSystemSignature);

	Gep::Signature windowSystemSignature; // no requirements will run on all entities
	em.SetSystemSignature<Client::WindowSystem>(windowSystemSignature);

	OUT_TYPE_HASH(Material);
	OUT_TYPE_HASH(Transform);
	OUT_TYPE_HASH(Identification);
	OUT_TYPE_HASH(RigidBody);

	em.Init();
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	/// ECS testing
	///////////////////////////////////////////////////////////////////////////////////////////////

	glm::vec3 gravity = { 0.0, 0.5, 0.0 };


	// entity 1 //////////////////////////////
	Gep::Entity entity1 = em.CreateEntity();
	em.AddComponent(entity1, Client::Transform
		{
			.position = glm::vec3(0, 0, 0),
			.scale = glm::vec3(5, 5, 5),
			.rotationAxis = glm::vec3(0, 1, 0),
			.rotationAmount = 0
		});
	em.AddComponent(entity1, Client::RigidBody
		{
			.velocity = {0, 0, 0},
			.acceleration = {0, 0, 0},
			.rotationalVelocity = 12,
			.rotationAxis = {0, 0, 0}
		});
	em.AddComponent(entity1, Client::Material
		{
			.diff_coeff = { 0.5, 1, 0.5 },
			.spec_coeff = { 0.5, 0.5, 0.5 },
			.spec_exponent = 5,
			.meshID = 0
		});
	em.AddComponent(entity1, Client::Identification
		{
			.name = "Sphere"
		});

	// entity 2 //////////////////////////////

	for (int i = 0; i < 10; i++)
	{
		Gep::Entity entity2 = em.CreateEntity();
		em.AddComponent(entity2, Client::Transform
			{
				.position = glm::vec3(0, 0, 0),
				.scale = glm::vec3(5, 5, 5),
				.rotationAxis = glm::vec3(0, 1, 0),
				.rotationAmount = 0
			});
		em.AddComponent(entity2, Client::RigidBody
			{
				.velocity = {0, 0, 0},
				.acceleration = {0, 0, 0},
				.rotationalVelocity = 12,
				.rotationAxis = {0, 0, 0}
			});
		em.AddComponent(entity2, Client::Material
			{
				.diff_coeff = { 0.5, 1, 0.5 },
				.spec_coeff = { 0.5, 0.5, 0.5 },
				.spec_exponent = 5,
				.meshID = 0
			});
		em.AddComponent(entity2, Client::Identification
			{
				.name = "Sphere"
			});
	}
	double dt = 0.1;
	while (em.IsRunning())
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		
		// TODO: there is a problem with updates being random because they are in an unordered map
		em.Update(dt);

		em.StartEvent<Gep::Event::EntityDestroyed>();

		auto endTime = std::chrono::high_resolution_clock::now();

		dt = std::chrono::duration<float, std::chrono::seconds::period>(endTime - startTime).count();
	}
}
