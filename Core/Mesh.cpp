/*****************************************************************//**
 * \file   Mesh.cpp
 * \brief  Mesh data structure used when rendering
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "Mesh.hpp"

namespace Gep
{
		Mesh::Mesh()
				: mVertices()
				, mFaces()
				, mEdges()
		{
		}

		Mesh::Mesh(const std::filesystem::path& meshFile)
				: mVertices()
				, mFaces()
				, mEdges()
		{
				Read(meshFile);
		}

		void Mesh::Read(const std::filesystem::path& meshFile)
		{
				// reads data in from a mesh data format
		}

		size_t Mesh::FaceCount() const
		{
				return mFaces.size();
		}

		NormalMesh::NormalMesh()
				: Mesh()
				, mNormals()
		{
		}
}

