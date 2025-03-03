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
    class OpenGLRenderer
    {
    public:
        OpenGLRenderer();
        ~OpenGLRenderer();

        // sets up the main shader program
        void LoadFragmentShader(const std::filesystem::path& shaderPath);
        void LoadVertexShader(const std::filesystem::path& shaderPath);
        void Compile();

        // loads resources into the renderer
        void LoadMesh(const std::string& name, const Mesh& mesh);

        // changes how the renderer will draw the next object
        void SetTexture(GLuint texture);
        void SetHighlight(bool highlight);
        void SetSolidColor(const glm::vec3& color);
        void SetCamera(const Camera& camera);
        void SetCamera(const glm::mat4& pers, const glm::mat4& view, const glm::vec3& eye);
        void SetMaterial(const glm::vec3& diffuseCoeff, const glm::vec3& specularCoeff, float specularExponent);
        void SetModel(const glm::mat4& modelingMatrix);
        void SetAmbientLight(const glm::vec3& color);
        std::vector<std::string> GetLoadedMeshes() const;
        std::vector<std::filesystem::path> GetLoadedTextures() const;
        
        void LoadIconTexture(const std::filesystem::path& iconPath);
        GLuint GetIconTexture(const std::string& extension);
        GLuint GetOrLoadIconTexture(const std::filesystem::path& iconPath);

        void LoadTexture(const std::filesystem::path& texturePath);
        GLuint GetTexture(const std::filesystem::path& texturePath);
        GLuint GetOrLoadTexture(const std::filesystem::path& texturePath);


        // completes the rendering of the object
        void DrawMesh(const std::string& meshID);

        void ToggleWireframes();
        void ToggleTextures();
        void UnloadMesh(const std::string& name);
        void BackfaceCull(bool enabled = true);

        // Start must be called before rendering and End must be called after rendering
        void Start(const glm::vec3& color = { 0, 0, 0 });
        void End();

        void AddLight(const glm::vec3& color, const glm::vec3& position, float intensity); // adds a light to the renderered, will be sent to the shader when DrawLights is called

        void DrawLights(); // will send all added lights to the shader
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
        bool mIsHighlighted = false;
        bool mUseSolidColor = false;
        glm::vec3 mSolidColor;

        std::unordered_map<std::string, MeshData> mMeshDatas;
        std::unordered_map<std::string, GLuint> mIconTextures;// icon extension -> texture id
        std::unordered_map<std::filesystem::path, GLuint> mTextures;

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
