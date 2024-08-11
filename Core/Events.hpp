/*****************************************************************//**
 * \file   EntityDestroyed.hpp
 * \brief  A bunch of predefined events
 * 
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

namespace Gep
{
	namespace Event
	{
		struct EntityDestroyed
		{
			Entity entity;
		};

		struct EntityCreated
		{
			Entity entity;
		};
	}
}
