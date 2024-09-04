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
#include <gtc/quaternion.hpp>

// backend
#include <System.hpp>
#include <EngineManager.hpp>
#include <Affine.hpp>

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
		{
		};

		~PhysicsSystem() = default;

		void Update(float dt)
		{
			std::unordered_set<Gep::Entity>& entities = mManager.GetEntities<Transform, RigidBody>();
			for (Gep::Entity entity : entities)
			{
				Transform& transform = mManager.GetComponent<Transform>(entity);
				RigidBody& rigidBody = mManager.GetComponent<RigidBody>(entity);
				
				transform.position += rigidBody.velocity * dt;				
				rigidBody.velocity += rigidBody.acceleration * dt;

				transform.rotation += rigidBody.rotationalVelocity * dt;				
				rigidBody.rotationalVelocity += rigidBody.rotationalAcceleration * dt;
			}
		}

		void EntityDestroyed(const Gep::Event::EntityDestroyed& eventData)
		{
			std::cout << "Physics system just got the entity destroyed event" << std::endl;
		};

		void KeyPressed(const Gep::Event::KeyPressed& eventData)
		{
			if (eventData.action == 1)
			{
				std::cout << "pressed down" << std::endl;
			}

			if (eventData.action == 0)
			{
				std::cout << "released" << std::endl;
			}

			if (eventData.action == 2)
			{
				std::cout << "held" << std::endl;
			}
		}
	};
}
