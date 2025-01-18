/*****************************************************************//**
 * \file   Renderer.hpp
 * \brief  Base interface for the type of rendering being performed
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include <glew.h>
#include <glm.hpp>
#include <Mesh.hpp>
#include <Camera.hpp>
#include <CompactArray.hpp>
#include <ShaderProgram.hpp>

namespace Gep
{
		class IRenderer
		{
		public:
				IRenderer();
				~IRenderer();

				virtual void LoadFragmentShader(const std::filesystem::path& shaderPath) final;
				virtual void LoadVertexShader(const std::filesystem::path& shaderPath) final;
				virtual void Compile() final;
				virtual std::uint64_t LoadMesh(const NormalMesh& mesh);
				virtual void UnloadMesh(std::uint64_t meshID);
				virtual void BackfaceCull(bool enabled = true) final;
				virtual void Clear(const glm::vec3& color = { 0, 0, 0 }) final;
				virtual void SetCamera(const Camera& camera) final;
				virtual void SetCamera(const glm::mat4& pers, const glm::mat4& view, const glm::vec4& eye);
				virtual void SetModel(const glm::mat4& modelingMatrix) final;
				virtual void SetMaterial(const glm::vec3& diffuseCoeff, const glm::vec3& specularCoeff, float specularExponent) final;
				virtual void CreateLight(const std::uint8_t lightID, const glm::vec3& position, const glm::vec3& color);
				virtual void SetAmbientLight(const glm::vec3& color);
				virtual void DrawMesh(std::uint64_t meshID) const;

		private:
				GLuint LoadShader(GLenum shaderType, const std::filesystem::path& shaderPath);

		private:
				struct MeshData
				{
				public:
						MeshData();
						~MeshData();

						void GenVertexBuffer(const Mesh& mesh);
						void GenNormalBuffer(const NormalMesh& mesh);
						void GenFaceBuffer(const Mesh& mesh);
						void BindBuffers();
						void DeleteBuffers();

				public:
						GLuint mVertexArrayObject;
						GLuint mVertexBuffer;
						GLuint mNormalBuffer;
						GLuint mFaceBuffer;
						size_t mFaceCount;
						size_t mEdgeCount;
				};

		private:
				ShaderProgram mProgram;
				compact_array<MeshData> mMeshDatas;
		};
}
