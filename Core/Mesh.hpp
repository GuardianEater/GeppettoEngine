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

namespace Gep
{
	/**
	 * Typical Mesh.
	 */
	class Mesh
	{
	public:
		Mesh()
			: mVertices()
			, mFaces()
			, mEdges()
		{}

		Mesh(const std::filesystem::path& meshFile)
			: mVertices()
			, mFaces()
			, mEdges()
		{
			Read(meshFile);
		}

		void Read(const std::filesystem::path& meshFile)
		{
			// reads data in from a mesh data format
		}

		size_t FaceCount() const
		{
			return mFaces.size();
		}

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
		NormalMesh() 
			: Mesh()
			, mNormals()
		{}

		using Normal = glm::vec<4, float>;

	public:
		std::vector<Normal> mNormals;
	};
}
