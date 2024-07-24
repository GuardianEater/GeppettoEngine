/*****************************************************************//**
 * \file   PhysicsSystem.hpp
 * \brief  test physics simulation
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <System.hpp>
#include <EngineManager.hpp>

// external

#include <glm.hpp>

namespace Client
{
	struct Gravity
	{
		glm::dvec3 force;
	};

	struct RigidBody
	{
		glm::dvec3 velocity;
		glm::dvec3 acceleration;
	};

	struct Transform
	{
		glm::dvec3 position;
		glm::dvec3 rotation;
		glm::dvec3 scale;
	};

	class PhysicsSystem : public Gep::ISystem
	{
	public:
		PhysicsSystem(Gep::EngineManager& em)
			: ISystem(em)
		{};

		~PhysicsSystem() = default;

		void Init() {};

		void Update(double dt)
		{
			for (Gep::Entity entity : mEntities)
			{
				RigidBody& rigidBody = mManager.GetComponent<RigidBody>(entity);
				Transform& transform = mManager.GetComponent<Transform>(entity);

				// Forces
				const Gravity& gravity = mManager.GetComponent<Gravity>(entity);

				transform.position += rigidBody.velocity * dt;

				rigidBody.velocity += gravity.force * dt;
			}
		}
	};
}
