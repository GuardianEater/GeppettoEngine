/*****************************************************************//**
 * \file   ScriptingSystem.hpp
 * \brief  allows adding scripts to entities
 * 
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <System.hpp>

#include <Script.hpp>

//#define SOL_ALL_SAFETIES_ON 1
#include <sol\sol.hpp>

namespace Client
{
	class ScriptingSystem : public Gep::ISystem
	{
	public:
		ScriptingSystem(Gep::EngineManager& em)
			: ISystem(em)
		{}

		sol::state mLua;

		void Initialize()
		{
			mLua.open_libraries();

			mLua.new_usertype<glm::vec3>("Vec3",
				"x", &glm::vec3::x,
				"y", &glm::vec3::y,
				"z", &glm::vec3::z
			);

			mLua.new_usertype<Client::Transform>("Transform",
				"position", &Transform::position,
				"scale", &Transform::scale,
				"rotation", &Transform::rotation
			);

			mLua.new_usertype<Client::RigidBody>("RigidBody",
				"velocity", &RigidBody::velocity,
				"acceleration", &RigidBody::acceleration,
				"rotational", &RigidBody::rotationalVelocity
			);
		}

		void Update(float dt)
		{
			std::unordered_set<Gep::Entity>& entities = mManager.GetEntities<Script>();
			for (Gep::Entity entity : entities)
			{
				Script& script = mManager.GetComponent<Client::Script>(entity);

				if (mManager.HasComponent<Transform>(entity))
				{
					mLua["Transform"] = &mManager.GetComponent<Transform>(entity);
				}
				if (mManager.HasComponent<RigidBody>(entity))
				{
					mLua["RigidBody"] = &mManager.GetComponent<RigidBody>(entity);
				}

				mLua.script(script.data, sol::script_pass_on_error);
			}
		}
	};
}
