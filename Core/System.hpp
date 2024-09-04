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
			: mManager(em)
		{}

		EngineManager& mManager;

		//std::unordered_set<Entity> mEntities;
	};
}
