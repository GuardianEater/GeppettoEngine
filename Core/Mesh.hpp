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
		class Mesh
		{
		public:
				Mesh();
				Mesh(const std::filesystem::path& meshFile);

				void Read(const std::filesystem::path& meshFile);
				size_t FaceCount() const;

		public:
				using Face = glm::vec<3, GLuint>;
				using Edge = glm::vec<2, GLuint>;
				using Vertex = glm::vec<4, float>;

		public:
				std::vector<Vertex> mVertices;
				std::vector<Face> mFaces;
				std::vector<Edge> mEdges;
		};

		/**
		 * Mesh that also contains a normal array.
		 */
		class NormalMesh : public Mesh
		{
		public:
				NormalMesh();

				using Normal = glm::vec<4, float>;

		public:
				std::vector<Normal> mNormals;
		};
}

