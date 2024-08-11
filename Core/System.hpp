/*****************************************************************//**
 * \file   SystemManager.hpp
 * \brief  The class that all systems should inherit from
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <Events.hpp>

namespace Gep
{
	class EngineManager;

	class ISystem
	{
	public:
		ISystem() = delete;

		explicit ISystem(EngineManager& em)
			: mEntities()
			, mManager(em)
		{}

		void _EntityDestroyed(const Event::EntityDestroyed& eventData)
		{

		};
		void _EntityCreated(const Event::EntityCreated& eventData) {};

		EngineManager& mManager;

		std::unordered_set<Entity> mEntities;

		// this is not good because it will call functions that potentially doing nothing, wasting alot of CPU cycles
		virtual void Init() {};
		virtual void Update(float dt) {};
	};
}
