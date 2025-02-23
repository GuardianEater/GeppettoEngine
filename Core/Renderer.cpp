/*****************************************************************//**
 * \file   Renderer.cpp
 * \brief  Base interface for the type of rendering being performed
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "pch.hpp"

#include "Renderer.hpp"

namespace Gep
{
    enum GLVertexAttributeLocation : GLint
    {
        Position,
        Normal,
        TexCoord
    };
    enum GLUniformLocation : GLint
    {
        Perspective, // perspective projection matrix
        ViewMatrix,
        ModelMatrix,
        NormalMatrix,
        Eye,
        DiffuseCoefficient,
        SpecularCoefficient,
        SpecularExponent,
        AmbientColor,
        TextureSampler,
        UseTexture,

        LightCount,
        IsSolidColor,
        SolidColor,

        IsHighlighted,
    };

    OpenGLRenderer::OpenGLRenderer()
        : mProgram()
        , mMeshDatas()
    {
    }

    OpenGLRenderer::~OpenGLRenderer()
    {
        mMeshDatas.clear();
    }

    void OpenGLRenderer::LoadFragmentShader(const std::filesystem::path& shaderPath)
    {
        mProgram.LoadFragmentShader(shaderPath);
    }

    void OpenGLRenderer::LoadVertexShader(const std::filesystem::path& shaderPath)
    {
        mProgram.LoadVertexShader(shaderPath);
    }

    void OpenGLRenderer::Compile()
    {
        mProgram.Compile();
        SetUpLightSSBO();
    }

    void OpenGLRenderer::LoadMesh(const std::string& name, const Mesh& mesh)
    {
        if (mMeshDatas.find(name) != mMeshDatas.end())
        {
            Gep::Log::Error("Cannot load mesh: [", name, "] a mesh with that name has already been loaded");
            return;
        }

        MeshData& meshData = mMeshDatas[name];

        meshData.GenVertexBuffer(mesh);
        meshData.GenFaceBuffer(mesh);
        meshData.BindBuffers();
        meshData.mEdgeCount = mesh.mEdges.size();
    }

    void OpenGLRenderer::LoadImage(const std::string& name, const std::filesystem::path& imagePath)
    {
        if (mTextures.contains(name)) {
            Gep::Log::Error("Cannot load image: [", name, "] that name has already been loaded");
            return;
        }

        if (!std::filesystem::exists(imagePath)) {
            Gep::Log::Error("Cannot load image: [", imagePath.string(), "] the file does not exist");
            return;
        }

        int width, height, channels;
        if (!stbi_info(imagePath.string().c_str(), &width, &height, &channels)) {
            Gep::Log::Error("Failed to get image info: [", imagePath.string(), "]");
            return;
        }

        int required_channels = 4; // Force RGBA
        unsigned char* image = stbi_load(imagePath.string().c_str(), &width, &height, &channels, required_channels);
        if (!image) {
            Gep::Log::Error("Failed to load image: [", imagePath.string(), "]");
            return;
        }

        GLuint& texture = mTextures[name];
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Ensure proper alignment
        GLenum format = (required_channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);

        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
        stbi_image_free(image);
    }


    void OpenGLRenderer::ToggleWireframes()
    {
        mWireframeMode = !mWireframeMode;
    }

    void OpenGLRenderer::UnloadMesh(const std::string& name)
    {
        if (mMeshDatas.find(name) == mMeshDatas.end())
        {
            Gep::Log::Error("Cannot unload mesh: [", name, "] a mesh with that name has not been loaded");
            return;
        }

        MeshData& meshData = mMeshDatas.at(name);
        meshData.DeleteBuffers();
        mMeshDatas.erase(name);
    }

    void OpenGLRenderer::BackfaceCull(bool enabled)
    {
        if (enabled)
        {
            glEnable(GL_CULL_FACE);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }
    }

    void OpenGLRenderer::Start(const glm::vec3& color)
    {
        //glClearColor(color.r, color.g, color.b, 1);
        //glClearDepth(1);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void OpenGLRenderer::SetCamera(const Camera& camera)
    {
        const glm::mat4 pers = camera.GetPerspective();
        const glm::mat4 view = camera.GetView();
        const glm::vec4 eye = camera.GetEyePosition();

        glUseProgram(mProgram.GetProgramID());
        glUniformMatrix4fv(0, 1, false, &pers[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniform4fv(4, 1, &eye[0]);
        glUseProgram(0);
    }

    void OpenGLRenderer::SetTexture(const std::string& textureName)
    {
        glUseProgram(mProgram.GetProgramID());

        if (!mTextures.contains(textureName))
        {
            Gep::Log::Error("Cannot set texture: [", textureName, "] a texture with that name has not been loaded");
            glBindTexture(GL_TEXTURE_2D, 0);
            return;
        }

        glBindTexture(GL_TEXTURE_2D, mTextures.at(textureName));
        mUseTextures = true;

        glUseProgram(0);
    }

    void OpenGLRenderer::SetHighlight(bool highlight)
    {
        mIsHighlighted = highlight;
    }

    void OpenGLRenderer::SetSolidColor(const glm::vec3& color)
    {
        glUseProgram(mProgram.GetProgramID());
        glUniform1i(GLUniformLocation::IsSolidColor, 1);
        glUniform3fv(GLUniformLocation::SolidColor, 1, &color[0]);
        glUseProgram(0);
    }

    void OpenGLRenderer::SetCamera(const glm::mat4& pers, const glm::mat4& view, const glm::vec3& eye)
    {
        glUseProgram(mProgram.GetProgramID());
        glUniformMatrix4fv(GLUniformLocation::Perspective, 1, false, &pers[0][0]);
        glUniformMatrix4fv(GLUniformLocation::ViewMatrix, 1, false, &view[0][0]);

        const glm::vec4 eye4 = glm::vec4(eye, 1);
        glUniform4fv(GLUniformLocation::Eye, 1, &eye4[0]);
        glUseProgram(0);
    }

    void OpenGLRenderer::SetModel(const glm::mat4& modelingMatrix)
    {
        glm::mat4 normal = glm::mat4(glm::mat3(affine_inverse(modelingMatrix)));

        glUseProgram(mProgram.GetProgramID());
        glUniformMatrix4fv(GLUniformLocation::ModelMatrix, 1, false, &modelingMatrix[0][0]);
        glUniformMatrix4fv(GLUniformLocation::NormalMatrix, 1, true, &normal[0][0]);
        glUseProgram(0);
    }

    void OpenGLRenderer::SetMaterial(const glm::vec3& diffuseCoeff, const glm::vec3& specularCoeff, float specularExponent)
    {
        glUseProgram(mProgram.GetProgramID());
        glUniform3fv(GLUniformLocation::DiffuseCoefficient, 1, &diffuseCoeff[0]);
        glUniform3fv(GLUniformLocation::SpecularCoefficient, 1, &specularCoeff[0]);
        glUniform1fv(GLUniformLocation::SpecularExponent, 1, &specularExponent);
        glUseProgram(0);
    }

    // toggle textures
    void OpenGLRenderer::ToggleTextures()
    {
        mUseTextures = !mUseTextures;
    }

    void OpenGLRenderer::SetAmbientLight(const glm::vec3& color)
    {
        glUseProgram(mProgram.GetProgramID());
        glUniform3fv(GLUniformLocation::AmbientColor, 1, &color[0]);
        glUseProgram(0);
    }

    std::vector<std::string> OpenGLRenderer::GetLoadedMeshes() const
    {
        std::vector<std::string> meshes;

        for (const auto& [name, _] : mMeshDatas)
        {
            meshes.push_back(name);
        }

        return meshes;
    }

    void OpenGLRenderer::DrawMesh(const std::string& meshName)
    {
        if (mMeshDatas.find(meshName) == mMeshDatas.end())
        {
            Gep::Log::Error("Cannot draw mesh: [", meshName, "] a mesh with that name has not been loaded");
            return;
        }

        const MeshData& md = mMeshDatas.at(meshName);
        constexpr std::uint64_t faceSize = sizeof(Mesh::Face) / sizeof(GLuint);

        glUseProgram(mProgram.GetProgramID());
        glBindVertexArray(md.mVertexArrayObject);
        glUniform1i(GLUniformLocation::UseTexture, mUseTextures);

        // If outlining is enabled, render the outline first
        if (mIsHighlighted)
        {
            glUniform1i(GLUniformLocation::IsHighlighted, 1);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, faceSize * md.mFaceCount, GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glUniform1i(GLUniformLocation::IsHighlighted, 0);
        }

        if (mWireframeMode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, faceSize * md.mFaceCount, GL_UNSIGNED_INT, 0);
        if (mWireframeMode) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glUniform1i(GLUniformLocation::IsSolidColor, 0); // reset solid color

        //glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        mUseTextures = false;
    }

    void OpenGLRenderer::End()
    {
        mLightData.clear();
    }

    void OpenGLRenderer::AddLight(const glm::vec3& color, const glm::vec3& position, float intensity)
    {
        LightData& data = mLightData.emplace_back();

        data.position = position;
        data.color = color;
        data.intensity = intensity;
    }

    void OpenGLRenderer::SetUpLightSSBO()
    {
        glGenBuffers(1, &mLightSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mLightSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 1 * sizeof(LightData), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mLightSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void OpenGLRenderer::DrawLights()
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mLightSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, mLightData.size() * sizeof(LightData), mLightData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glUseProgram(mProgram.GetProgramID());
        glUniform1i(GLUniformLocation::LightCount, static_cast<int>(mLightData.size()));
        glUseProgram(0);
    }

    GLuint OpenGLRenderer::LoadShader(GLenum shaderType, const std::filesystem::path& shaderPath)
    {
        std::string source;

        std::ifstream inFile(shaderPath);
        assert(!(!inFile.is_open()) && "Failed to open shader file");

        source.assign((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();

        GLuint shaderID = glCreateShader(shaderType);
        const char* c_source = source.c_str();
        glShaderSource(shaderID, 1, &c_source, 0);
        glCompileShader(shaderID);

#ifdef _DEBUG
        GLint errorValue = 0;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &errorValue);
        if (!errorValue)
        {
            std::string message;
            message.resize(1024);
            glGetShaderInfoLog(shaderID, message.capacity(), 0, message.data());
            std::cout << "Failed to Compile Shader " << shaderPath.string() << '\n' << message << std::endl;
            throw std::runtime_error("Failed to Compile Shader");
        }
#endif // _DEBUG
        return shaderID;
    }

    OpenGLRenderer::MeshData::MeshData()
        : mVertexArrayObject(num_max<GLuint>())
        , mVertexBuffer(num_max<GLuint>())
        , mFaceBuffer(num_max<GLuint>())
        , mFaceCount(num_max<size_t>())
        , mEdgeCount(num_max<size_t>())
    {
    }

    OpenGLRenderer::MeshData::~MeshData()
    {
    }

    void OpenGLRenderer::MeshData::GenVertexBuffer(const Mesh& mesh)
    {
        glGenBuffers(1, &mVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.mVertices.size(), mesh.mVertices.data(), GL_STATIC_DRAW);
    }

    void OpenGLRenderer::MeshData::GenFaceBuffer(const Mesh& mesh)
    {
        glGenBuffers(1, &mFaceBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mFaceBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Mesh::Face) * mesh.mFaces.size(), mesh.mFaces.data(), GL_STATIC_DRAW);

        mFaceCount = mesh.mFaces.size();
    }

    void OpenGLRenderer::MeshData::BindBuffers()
    {
        glGenVertexArrays(1, &mVertexArrayObject);
        glBindVertexArray(mVertexArrayObject);

        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glVertexAttribPointer(GLVertexAttributeLocation::Position, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(GLVertexAttributeLocation::Normal, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(GLVertexAttributeLocation::TexCoord, 2, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mFaceBuffer);

        glBindVertexArray(0);
    }

    void OpenGLRenderer::MeshData::DeleteBuffers()
    {
        glDeleteBuffers(1, &mFaceBuffer);
        glDeleteBuffers(1, &mVertexBuffer);
        glDeleteVertexArrays(1, &mVertexArrayObject);

#ifdef _DEBUG
        mVertexArrayObject = num_max<GLuint>();
        mVertexBuffer = num_max<GLuint>();
        mFaceBuffer = num_max<GLuint>();
        mFaceCount = num_max<GLuint>();
#endif // _DEBUG
    }
}
