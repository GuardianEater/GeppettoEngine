/*****************************************************************//**
 * \file   CameraComponent.hpp
 * \brief  Component that stores camera data
 * 
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <glm.hpp>

namespace Client
{
	struct Camera
	{
		glm::vec3 viewport; // the width and height of the viewport as well as depth

		glm::vec3 back;  // vector pointing out the back of the camera
		glm::vec3 right; // vector pointing to the right of the camera
		glm::vec3 up;    // vector pointer out the top of the camera

		float nearPlane; // how far away to start rendering
		float farPlane;  // how far away to stop rendering
	};
}
