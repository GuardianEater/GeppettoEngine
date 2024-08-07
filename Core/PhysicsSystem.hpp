/*****************************************************************//**
 * \file   PhysicsSystem.hpp
 * \brief  test physics simulation
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

// core
#include <Core.hpp>
#include <glm.hpp>

// backend
#include <System.hpp>
#include <EngineManager.hpp>

// client

#include <Transform.hpp>
#include <RigidBody.hpp>

namespace Client
{
	class PhysicsSystem : public Gep::ISystem
	{
	public:
		PhysicsSystem(Gep::EngineManager& em)
			: ISystem(em)
		{};

		~PhysicsSystem() = default;

		void Init() {};

		void Update(float dt)
		{
			for (Gep::Entity entity : mEntities)
			{
				const glm::vec4 gravity = {0, -1, 0, 0};

				Transform& transform = mManager.GetComponent<Transform>(entity);
				RigidBody& rigidBody = mManager.GetComponent<RigidBody>(entity);
				// Forces
				
				transform.position += rigidBody.velocity * dt;

				rigidBody.velocity += gravity * dt;
			}
		}
	};
}
