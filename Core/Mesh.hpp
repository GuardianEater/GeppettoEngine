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
		struct Face
		{
			GLuint index1;
			GLuint index2;
			GLuint index3;
		};

		struct Edge
		{
			GLuint index1;
			GLuint index2;
		};

	public:
		std::vector<glm::vec4> mVertices;
		std::vector<glm::vec4> mNormals;
		std::vector<Face> mFaces;
		std::vector<Edge> mEdges;
	};
}
