/*****************************************************************//**
 * \file   main.cpp
 * \brief  entry point for the engine
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include <Core.hpp>
#include <Renderer.hpp>

#include <EngineManager.hpp>

#include <PhysicsSystem.hpp>
#include <ImGuiSystem.hpp>
#include <WindowSystem.hpp>



int main()
{
	Gep::EngineManager em;

	// register all components
	em.RegisterComponent<Client::Gravity>();
	em.RegisterComponent<Client::RigidBody>();
	em.RegisterComponent<Client::Transform>();

	// register all systems
	em.RegisterSystem<Client::PhysicsSystem>();
	em.RegisterSystem<Client::WindowSystem>();
	em.RegisterSystem<Client::ImGuiSystem>();

	// this makes the physics system require an entity to have the listed components to use physics
	Gep::Signature physicsSystemSignature;
	physicsSystemSignature.set(em.GetComponentID<Client::Gravity>());
	physicsSystemSignature.set(em.GetComponentID<Client::RigidBody>());
	physicsSystemSignature.set(em.GetComponentID<Client::Transform>());
	em.SetSystemSignature<Client::PhysicsSystem>(physicsSystemSignature);

	em.Init();

	
	glm::vec3 gravity = { 0.0, 0.5, 0.0 };

	Gep::Entity entity1 = em.CreateEntity();

	em.AddComponent
	(
		entity1,
		Client::Gravity 
		{ 
			.force = glm::vec3(0.0f, -0.5, 0.0f)
		}
	);

	em.AddComponent
	(
		entity1,
		Client::RigidBody
		{
			.velocity = glm::vec3(0.0f, 0.0f, 0.0f),
			.acceleration = glm::vec3(0.0f, 0.0f, 0.0f)
		}
	);

	em.AddComponent
	(
		entity1,
		Client::Transform
		{
			.position = glm::vec3(),
			.rotation = glm::vec3(),
			.scale = glm::vec3(5, 5, 5)
		}
	);
	
	Gep::IRenderer renderer;
	renderer.LoadVertexShader("Core\\assets\\shaders\\PhongRender.vert");
	renderer.LoadFragmentShader("Core\\assets\\shaders\\PhongRender.frag");
	renderer.Compile();

	//renderer.LoadMesh()

	double dt = 0.1;
	while (em.IsRunning())
	{
		auto startTime = std::chrono::high_resolution_clock::now();

		em.Update(dt);

		std::cout << em.GetComponent<Client::Transform>(entity1).position.y << std::endl;

		auto stopTime = std::chrono::high_resolution_clock::now();

		dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();
	}
}
