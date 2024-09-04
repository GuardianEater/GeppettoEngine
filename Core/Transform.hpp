/*****************************************************************//**
 * \file   Transform.hpp
 * \brief  transform component, stores position data
 * 
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <glm.hpp>

namespace Client
{
	struct Transform
	{
		glm::vec3 position;
		glm::vec3 scale;
		glm::vec3 rotation;
	};
}
