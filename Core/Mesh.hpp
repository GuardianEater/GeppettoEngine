/*****************************************************************//**
 * \file   Mesh.hpp
 * \brief  Mesh data structure used when rendering
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include <glew.h>
#include <glm.hpp>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <vector>

namespace Gep
{
		/**
		 * Typical Mesh.
		 */
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };

		struct Mesh
		{
				using Face   = glm::vec<3, GLuint>;
				using Edge   = glm::vec<2, GLuint>;

				std::vector<Vertex> mVertices;
				std::vector<Face> mFaces;
				std::vector<Edge> mEdges;
		};
}

