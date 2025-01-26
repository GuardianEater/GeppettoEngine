/*****************************************************************//**
 * \file   SystemManager.hpp
 * \brief  The class that all systems should inherit from
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

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

		virtual ~ISystem() = default;

		// called once at the start of the engine
		virtual void Initialize() {};

		// called once per frame in the order they were registered
		virtual void FrameStart() {};

		// called once per frame in the order they were registered
		virtual void Update(float dt) {};

		// called at the end of the frame in the reverse order they were registered
		virtual void FrameEnd() {};

		// called once at the end of the engine
		virtual void Exit() {};

		EngineManager& mManager;

		//std::unordered_set<Entity> mEntities;
	};
}
