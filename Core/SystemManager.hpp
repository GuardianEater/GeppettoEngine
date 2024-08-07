/*****************************************************************//**
 * \file   SystemManager.hpp
 * \brief  class for managing systems
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <System.hpp>

namespace Gep
{
	// forwards engine manager so each system has acess to the engine manager
	class EngineManager;

	class SystemManager
	{
	public:

		SystemManager(EngineManager& em)
			: mManager(em)
			, mSignatures()
			, mSystems()
		{}

		void Init()
		{
			for (const auto& [id, system] : mSystems)
			{
				system->Init();
			}
		}

		void Update(float dt)
		{
			for (const auto& [id, system] : mSystems)
			{
				system->Update(dt);
			}
		}

		template<typename SystemType>
		void RegisterSystem()
		{
			static_assert(std::is_base_of<ISystem, SystemType>::value, "SystemType must inherit from ISystem");

			const std::uint64_t typeID = typeid(SystemType).hash_code();

			mSystems[typeID] = std::make_shared<SystemType>(mManager);
		}

		template<typename SystemType>
		void SetSignature(Signature signature)
		{
			const std::uint64_t typeID = typeid(SystemType).hash_code();

			mSignatures[typeID] = signature;
		}

		void Event_EntityDestroyed(Entity entity)
		{
			for (const auto& [id, system] : mSystems)
			{
				system->mEntities.erase(entity);
			}
		}

		void Event_EntitySignatureChanged(Entity entity, Signature entityNewSignature)
		{
			for (const auto& [id, system] : mSystems)
			{
				Signature systemSignature = mSignatures[id];

				// checks if the entities signature is the same as the systems signature, if so add the entity to the system
				Signature testSignature = entityNewSignature & systemSignature;
				if (testSignature == systemSignature)
				{
					system->mEntities.insert(entity);
				}
				else
				{
					system->mEntities.erase(entity);
				}
			}
		}

	private:
		// the signatures of all of the systems maps the typeid of a system to its signature
		std::unordered_map<std::uint64_t, Signature> mSignatures;

		// maps the typeid of a system to the actual system class
		std::unordered_map<std::uint64_t, std::shared_ptr<ISystem>> mSystems;

		// reference to the manager that this SystemManager belongs to
		EngineManager& mManager;
	};
}
