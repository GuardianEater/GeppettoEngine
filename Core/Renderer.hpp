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
#include <ShaderProgram.hpp>

#include "stb_image.h"
#include "Logger.hpp"

namespace Gep
{
		class IRenderer
		{
		public:
				IRenderer();
				~IRenderer();

				virtual void LoadFragmentShader(const std::filesystem::path& shaderPath) final;
				virtual void LoadVertexShader(const std::filesystem::path& shaderPath) final;
				virtual void LoadMesh(const std::string& name, const Mesh& mesh);
        virtual void LoadImage(const std::string& name, const std::filesystem::path& imagePath);
        virtual void SetTexture(const std::string& textureName) final;
				virtual void SetHighlight();
        virtual void SetSolidColor(const glm::vec3& color) final;
        virtual void ToggleWireframes() final;
        virtual void ToggleTextures() final;
				virtual void Compile() final;
				virtual void UnloadMesh(const std::string& name);
				virtual void BackfaceCull(bool enabled = true) final;
				virtual void Clear(const glm::vec3& color = { 0, 0, 0 }) final;
				virtual void SetCamera(const Camera& camera) final;
				virtual void SetCamera(const glm::mat4& pers, const glm::mat4& view, const glm::vec4& eye);
				virtual void SetModel(const glm::mat4& modelingMatrix) final;
				virtual void SetMaterial(const glm::vec3& diffuseCoeff, const glm::vec3& specularCoeff, float specularExponent) final;
				virtual void SetAmbientLight(const glm::vec3& color);
				virtual void DrawMesh(const std::string& meshID);
				void AddLight(const glm::vec3& color,  const glm::vec3& position, float intensity);
				void DrawLights();

		private:
				GLuint LoadShader(GLenum shaderType, const std::filesystem::path& shaderPath);
				void SetUpLightSSBO();

				GLuint mSSBO;

		private:
				struct MeshData
				{
				public:
						MeshData();
						~MeshData();

						void GenVertexBuffer(const Mesh& mesh);
						void GenFaceBuffer(const Mesh& mesh);
						void BindBuffers();
						void DeleteBuffers();

				public:
						GLuint mVertexArrayObject;
						GLuint mVertexBuffer;
						GLuint mFaceBuffer;
						size_t mFaceCount;
						size_t mEdgeCount;
				};

		private:
				ShaderProgram mProgram;
				//keyed_vector<MeshData> mMeshDatas;
        bool mWireframeMode = false;
        bool mUseTextures = false;
        bool mIsOutlinePass = false;
        bool mUseSolidColor = false;
        glm::vec3 mSolidColor;

        std::unordered_map<std::string, GLuint> mTextures;
        std::unordered_map<std::string, MeshData> mMeshDatas;

				struct LightData
				{
						alignas(16) glm::vec3 position;
						alignas(16) glm::vec3 color;
            float intensity;
				};

				GLuint mLightSSBO;
				std::vector<LightData> mLightData;
		};
}
