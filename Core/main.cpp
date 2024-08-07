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

int main()
{
	Gep::EngineManager em;

	// register all components
	em.RegisterComponent<Client::RigidBody>();
	em.RegisterComponent<Client::Material>();
	em.RegisterComponent<Client::Transform>();

	// register all systems
	em.RegisterSystem<Client::PhysicsSystem>();
	em.RegisterSystem<Client::WindowSystem>();
	//em.RegisterSystem<Client::RenderSystem>();
	em.RegisterSystem<Client::ImGuiSystem>();

	// this makes the physics system require an entity to have the listed components to use physics
	Gep::Signature physicsSystemSignature;
	physicsSystemSignature.set(em.GetComponentID<Client::RigidBody>());
	physicsSystemSignature.set(em.GetComponentID<Client::Transform>());
	em.SetSystemSignature<Client::PhysicsSystem>(physicsSystemSignature);

	//Gep::Signature renderSystemSignature;
	//renderSystemSignature.set(em.GetComponentID<Client::Material>());
	//renderSystemSignature.set(em.GetComponentID<Client::Transform>());
	//em.SetSystemSignature<Client::RenderSystem>(renderSystemSignature);

	em.Init();
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	/// ECS testing
	///////////////////////////////////////////////////////////////////////////////////////////////

	glm::vec3 gravity = { 0.0, 0.5, 0.0 };

	Gep::Entity entity1 = em.CreateEntity();

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
			.scale = glm::vec3(5, 5, 5),
			.rotationAxis = glm::vec3(),
			.rotationAmount = 0
		}
	);

	em.AddComponent
	(
		entity1,
		Client::Material
		{
			.diff_coeff = { 0.5, 1, 0.5 },
			.spec_coeff = { 0.5, 0.5, 0.5 },
			.spec_exponent = 5,
			.meshID = 0
		}
	);

	
	///////////////////////////////////////////////////////////////////////////////////////////////
	/// Rendering testing
	///////////////////////////////////////////////////////////////////////////////////////////////

	Gep::IRenderer renderer;
	renderer.LoadVertexShader("assets\\shaders\\PhongRender.vert");
	renderer.LoadFragmentShader("assets\\shaders\\PhongRender.frag");
	//renderer.LoadVertexShader("assets\\shaders\\basic.vert");
	//renderer.LoadFragmentShader("assets\\shaders\\basic.frag");
	renderer.Compile();

	struct Object 
	{
		size_t meshID;
		glm::vec3 diffuse_coef;
		glm::vec3 specular_coef;
		float specular_exp;
		glm::mat4 model;
		Object()
			: meshID(-1)
			, diffuse_coef(0)
			, specular_coef(0)
			, specular_exp(0)
			, model(1)
		{}
	};

	std::vector<Object> objects;
	
	size_t sphereMeshID = renderer.LoadMesh(Gep::SphereMesh(20, 10));

	glm::vec4 EX = { 1, 0, 0, 0 };
	glm::vec4 EY = { 0, 1, 0, 0 };
	glm::vec4 EZ = { 0, 0, 1, 0 };

	Gep::Camera camera(glm::vec4(0, 0, 0 ,1) + 10.f * EZ, -EZ, EY, 80, 1, 0.1f, 100);

	// bottom ball
	Object& object1 = objects.emplace_back();
	object1.meshID = sphereMeshID;
	object1.diffuse_coef = { 0.5, 1, 0.5 };
	object1.specular_coef = { 0.5, 0.5, 0.5 };
	object1.specular_exp = 5;
	object1.model = Gep::scale_matrix(4) * Gep::translation_matrix({ 0, -2, 0 });

	// light
	Object& object2 = objects.emplace_back();
	object2.meshID = sphereMeshID;
	object2.diffuse_coef = { 0.9, 0.1, 0.1 };
	object2.specular_coef = { 0.5, 0.5, 0.5 };
	object2.specular_exp = 40;
	object2.model = Gep::scale_matrix(1) * Gep::translation_matrix({0, 5, 0});
	renderer.CreateLight(0, { 0, 5, 0 }, { 1, 0, 0 });


	renderer.BackfaceCull(true);
	renderer.SetAmbientLight({ 0.5, 0.5, 0.5 });

	double dt = 0.1;
	while (em.IsRunning())
	{
		auto startTime = std::chrono::high_resolution_clock::now();

		renderer.Clear({0, 0, 0});

		renderer.SetCamera(camera);

		objects[0].model = Gep::rotation_matrix(100 * dt, EY) * objects[0].model;
		objects[1].model = Gep::rotation_matrix(-100 * dt, EY) * objects[1].model;

		for (size_t i = 0; i < objects.size(); i++)
		{
			renderer.SetModel(objects[i].model);
			renderer.SetMaterial(objects[i].diffuse_coef, objects[i].specular_coef, objects[i].specular_exp);
			renderer.DrawMesh(objects[i].meshID);
		}
		
		em.Update(dt);

		////std::cout << em.GetComponent<Client::Transform>(entity1).position.y << std::endl;

		auto stopTime = std::chrono::high_resolution_clock::now();

		dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();
	}

	renderer.UnloadMesh(sphereMeshID);
}
