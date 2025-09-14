/*****************************************************************//**
 * \file   Renderer.cpp
 * \brief  Base interface for the type of rendering being performed
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "pch.hpp"

#include "OpenGLRenderer.hpp"
#include "Model.hpp"

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "shellapi.h"
#undef LoadImage
#undef min
#undef max

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
        IgnoreLight,
    };

    static HICON GetIcon(const std::filesystem::path& iconPath);
    static GLuint IconToTexture(HICON icon);
    static GLuint BitmapToTexture(HBITMAP bitmap);

    void OpenGLRenderer::AddModelFromFile(const std::string& path)
    {
        if (mModelHandles.contains(path))
        {
            Gep::Log::Error("Cannot load mesh: [", path, "] a mesh with that name has already been loaded");
            return;
        }

        Model model = Model::FromFile(path);

        ModelGPUHandle& modelHandle = mModelHandles[path]; // create a handle for this model, and sets its name to its path

        for (const auto& mesh : model.meshes)
        {
            MeshGPUHandle& meshHandle = modelHandle.meshHandles.emplace_back(); // create a handle for this mesh

            const std::filesystem::path& diffuseTexturePath = model.materials.at(mesh.materialIndex).diffuseTexturePath;
            std::filesystem::path root(path);
            root = root.parent_path();

            meshHandle.materialHandle.diffuseTexture = GetOrLoadTexture(root / diffuseTexturePath);
            meshHandle.GenVertexBuffer(mesh);
            meshHandle.GenIndexBuffer(mesh);
            meshHandle.BindBuffers();
        }
    }

    void OpenGLRenderer::AddModel(const std::string& name, const Gep::Model& model)
    {
        if (mModelHandles.contains(name))
        {
            Gep::Log::Error("Cannot load mesh: [", name, "] a mesh with that name has already been loaded");
            return;
        }

        ModelGPUHandle& modelHandle = mModelHandles[name];

        for (const Mesh& mesh : model.meshes)
        {
            MeshGPUHandle& meshHandle = modelHandle.meshHandles.emplace_back();

            meshHandle.materialHandle.diffuseTexture = GetErrorTexture();
            meshHandle.GenVertexBuffer(mesh);
            meshHandle.GenIndexBuffer(mesh);
            meshHandle.BindBuffers();
        }
    }

    bool OpenGLRenderer::IsMeshLoaded(const std::string& name) const
    {
        return mModelHandles.contains(name);
    }

    void OpenGLRenderer::SetShader(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath)
    {
        mPBRShader = std::make_unique<Shader>(Shader::FromFile(vertPath, fragPath));
    }

    void OpenGLRenderer::SetHighlightShader(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath)
    {
        mHighlightShader = std::make_unique<Shader>(Shader::FromFile(vertPath, fragPath));
    }

    void OpenGLRenderer::SetColorShader(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath)
    {
        mColorShader = std::make_unique<Shader>(Shader::FromFile(vertPath, fragPath));
    }

    void OpenGLRenderer::AddObject(const std::string& modelName, const ObjectGPUData& objectData)
    {
        mModelHandles.at(modelName).objectDatas.push_back(objectData);
    }

    void OpenGLRenderer::AddCamera(const CameraGPUData& uniforms)
    {
        mCameraUniforms.push_back(uniforms);
    }

    void OpenGLRenderer::AddLight(const LightGPUData& uniforms)
    {
        mLightUniforms.push_back(uniforms);
    }

    void OpenGLRenderer::CommitObjects()
    {
        for (auto& [modelName, modelHandle] : mModelHandles)
        {
            for (auto& obj : modelHandle.objectDatas)
            {
                if (obj.isWireframe)
                    modelHandle.wireframeObjectDatas.push_back(obj);
                else
                    modelHandle.regularObjectDatas.push_back(obj);
            }

            mObjectUniforms.insert(mObjectUniforms.end(),
                modelHandle.regularObjectDatas.begin(),
                modelHandle.regularObjectDatas.end()
            );

            mObjectUniforms.insert(mObjectUniforms.end(),
                modelHandle.wireframeObjectDatas.begin(),
                modelHandle.wireframeObjectDatas.end()
            );
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mObjectsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, mObjectUniforms.size() * sizeof(ObjectGPUData), mObjectUniforms.data(), GL_DYNAMIC_DRAW);
    }

    void OpenGLRenderer::CommitCameras()
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mCameraUniformsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, mCameraUniforms.size() * sizeof(CameraGPUData), mCameraUniforms.data(), GL_DYNAMIC_DRAW);
    }

    void OpenGLRenderer::CommitLights()
    {
        SetLightCount(mLightUniforms.size());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mLightUniformsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, mLightUniforms.size() * sizeof(LightGPUData), mLightUniforms.data(), GL_DYNAMIC_DRAW);
    }

    void OpenGLRenderer::SetCameraIndex(size_t index)
    {
        mPBRShader->SetUniform(0, static_cast<int>(index));
        mHighlightShader->SetUniform(0, static_cast<int>(index));
    }

    void OpenGLRenderer::SetLightCount(size_t count)
    {
        mPBRShader->SetUniform(2, static_cast<int>(count));
        mHighlightShader->SetUniform(2, static_cast<int>(count));
    }

    void OpenGLRenderer::ToggleWireframes()
    {
        mWireframeMode = !mWireframeMode;
    }

    void OpenGLRenderer::UnloadModel(const std::string& name)
    {
        if (!mModelHandles.contains(name))
        {
            Gep::Log::Error("Cannot unload mesh: [", name, "] a mesh with that name has not been loaded");
            return;
        }

        // aquire the model id from the name
        ModelGPUHandle& modelHandle = mModelHandles[name];

        // delete all meshes owned by the model
        for (MeshGPUHandle& meshHandle : modelHandle.meshHandles)
        {
            meshHandle.DeleteBuffers();
        }

        mModelHandles.erase(name);
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

    // toggle textures
    void OpenGLRenderer::ToggleTextures()
    {
        mTexturesEnabled = !mTexturesEnabled;
    }

    std::vector<std::string> OpenGLRenderer::GetLoadedMeshes() const
    {
        std::vector<std::string> meshes;

        for (const auto& [name, modelHandle] : mModelHandles)
        {
            meshes.emplace_back(name);
        }

        return meshes;
    }

    std::vector<std::filesystem::path> OpenGLRenderer::GetLoadedTextures() const
    {
        std::vector<std::filesystem::path> textures;

        for (const auto& [name, texture] : mTextures)
        {
            textures.push_back(name);
        }

        return textures;
    }

    const std::vector<std::string>& OpenGLRenderer::GetSupportedModelFormats() const
    {
        static std::vector<std::string> allowedExtensions = []() // initializes this vector with the extensions that work with assimp
            {
                std::string s;
                Assimp::Importer importer;
                importer.GetExtensionList(s);

                s.erase(std::remove(s.begin(), s.end(), '*'), s.end());

                std::vector<std::string> out;
                std::istringstream ss(s);
                std::string token;
                while (std::getline(ss, token, ';'))
                    if (!token.empty())
                        out.emplace_back(std::move(token));
                return out;
            }();

        return allowedExtensions;
    }

    const std::vector<std::string>& OpenGLRenderer::GetSupportedTextureFormats() const
    {
        static std::vector<std::string> allowedExtensions = { ".jpg", ".jpeg", ".png", ".bmp" };

        return allowedExtensions;
    }

    void OpenGLRenderer::LoadIconTexture(const std::filesystem::path& iconPath)
    {
        if (mIconTextures.contains(iconPath.extension().string()))
        {
            Gep::Log::Error("Cannot load icon: [", iconPath.string(), "] an icon with that extension has already been loaded");
            return;
        }

        HICON icon = GetIcon(iconPath);
        if (!icon)
        {
            Gep::Log::Error("Failed to load icon: [", iconPath.string(), "]");
            return;
        }

        GLuint texture = IconToTexture(icon);
        if (!texture)
        {
            Gep::Log::Error("Failed to convert icon to texture: [", iconPath.string(), "]");
            return;
        }

        mIconTextures[iconPath.extension().string()] = texture;
    }

    GLuint OpenGLRenderer::GetIconTexture(const std::string& extension)
    {
        if (!mIconTextures.contains(extension))
        {
            Gep::Log::Error("Cannot get icon texture: [", extension, "] an icon with that extension has not been loaded");
            return 0;
        }

        return mIconTextures.at(extension);
    }

    GLuint OpenGLRenderer::GetOrLoadIconTexture(const std::filesystem::path& iconPath)
    {
        if (!mIconTextures.contains(iconPath.extension().string()))
            LoadIconTexture(iconPath);

        // check if the icon path is a supported image format
        const std::vector<std::string>& supportedTextureFormats = GetSupportedTextureFormats();
        auto it = std::find(supportedTextureFormats.begin(), supportedTextureFormats.end(), iconPath.extension().string());

        // if the icon is a supported image, use the image itself
        if (it != supportedTextureFormats.end())
        {
            return GetOrLoadTexture(iconPath);
        }

        return mIconTextures.at(iconPath.extension().string());
    }

    void OpenGLRenderer::LoadTextureAsync(const std::filesystem::path& texturePath)
    {
        std::lock_guard<std::mutex> lock(mTextureLoadingMutex);

        if (mTextures.contains(texturePath)) {
            Gep::Log::Error("Cannot load texture: [", texturePath, "] has already been loaded");
            return;
        }

        if (!std::filesystem::exists(texturePath)) {
            Gep::Log::Error("Cannot load texture: [", texturePath.string(), "] does not exist");
            return;
        }

        mTextures[texturePath] = GetErrorTexture();

        std::thread([&]()
            {
                LoadTexture(texturePath);
            }).detach();
    }

    void OpenGLRenderer::LoadTexture(const std::filesystem::path& texturePath)
    {
        if (mTextures.contains(texturePath))
        {
            Gep::Log::Error("Failed to load texture: [", texturePath.string(), "] a texture with that name is already loaded.");
            return;
        }

        if (!std::filesystem::exists(texturePath))
        {
            Gep::Log::Error("Cannot load texture: [", texturePath.string(), "] does not exist");
            return;
        }

        int width, height, channels;
        if (!stbi_info(texturePath.string().c_str(), &width, &height, &channels)) {
            Gep::Log::Error("Failed to get texture info: [", texturePath.string(), "]");
            return;
        }

        int required_channels = 4; // Force RGBA
        unsigned char* image = stbi_load(texturePath.string().c_str(), &width, &height, &channels, required_channels);
        if (!image)
        {
            Gep::Log::Error("Failed to load texture: [", texturePath.string(), "]");
            return;
        }


        GLuint& texture = mTextures[texturePath];
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

    GLuint OpenGLRenderer::GetTexture(const std::filesystem::path& texturePath)
    {
        if (!mTextures.contains(texturePath))
        {
            Gep::Log::Error("Cannot get texture: [", texturePath, "] a texture with that name has not been loaded");
            return GetErrorTexture();
        }

        return mTextures.at(texturePath);
    }

    GLuint OpenGLRenderer::GetOrLoadTexture(const std::filesystem::path& texturePath)
    {
        if (!mTextures.contains(texturePath))
            LoadTexture(texturePath);

        if (!mTextures.contains(texturePath))
            return GetErrorTexture();

        return mTextures.at(texturePath);
    }

    void OpenGLRenderer::LoadErrorTexture(const std::filesystem::path& texturePath)
    {
        LoadTexture(texturePath);
        mErrorTexture = GetTexture(texturePath);
    }

    GLuint OpenGLRenderer::GetErrorTexture() const
    {
        if (!mErrorTexture)
        {
            Gep::Log::Error("Cannot get error texture: an error texture has not been loaded");
            return 0;
        }

        return mErrorTexture;
    }

    void OpenGLRenderer::Draw()
    {
        DrawRegular();
    }

    void OpenGLRenderer::End()
    {
        mLightUniforms.clear();
        mObjectUniforms.clear();
        mCameraUniforms.clear();
    }

    void OpenGLRenderer::SetUpLightSSBO()
    {
        glGenBuffers(1, &mLightUniformsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mLightUniformsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(LightGPUData) * 1, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mLightUniformsSSBO); // the number is the binding value of the buffer declared in the shader
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void OpenGLRenderer::SetUpObjectUniformsSSBO()
    {
        glGenBuffers(1, &mObjectsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mObjectsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ObjectGPUData) * 1, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mObjectsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void OpenGLRenderer::SetUpCameraUniformsSSBO()
    {
        glGenBuffers(1, &mCameraUniformsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mCameraUniformsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(CameraGPUData) * 1, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mCameraUniformsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void OpenGLRenderer::DrawRegular()
    {
        mPBRShader->Bind();

        size_t baseInstance = 0;
        for (auto& [modelName, modelHandle] : mModelHandles)
        {
            // normal draw
            for (const MeshGPUHandle& meshHandle : modelHandle.meshHandles)
            {
                glBindVertexArray(meshHandle.mVertexArrayObject);

                if (meshHandle.materialHandle.diffuseTexture != num_max<GLuint>())
                    glBindTexture(GL_TEXTURE_2D, meshHandle.materialHandle.diffuseTexture);

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glDrawElementsInstancedBaseInstance(
                    GL_TRIANGLES,
                    meshHandle.mIndexCount,
                    GL_UNSIGNED_INT,
                    0,
                    modelHandle.regularObjectDatas.size(),
                    baseInstance
                );

                glBindTexture(GL_TEXTURE_2D, 0);
            }
            baseInstance += modelHandle.regularObjectDatas.size();

            // --- Wireframe draw ---
            for (const MeshGPUHandle& meshHandle : modelHandle.meshHandles)
            {
                glBindVertexArray(meshHandle.mVertexArrayObject);

                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawElementsInstancedBaseInstance(
                    GL_TRIANGLES,
                    meshHandle.mIndexCount,
                    GL_UNSIGNED_INT,
                    0,
                    modelHandle.wireframeObjectDatas.size(),
                    baseInstance
                );
            }
            baseInstance += modelHandle.wireframeObjectDatas.size();

            modelHandle.objectDatas.clear();
            modelHandle.regularObjectDatas.clear();
            modelHandle.wireframeObjectDatas.clear();
        }

        mPBRShader->Unbind();
    }

    void OpenGLRenderer::AddWireframeObject(const std::string& modelName, const ObjectGPUData& objectData)
    {
    }

    HICON GetIcon(const std::filesystem::path& iconPath)
    {
        SHFILEINFO sfi{};
        if (SHGetFileInfo(iconPath.wstring().c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON))
        {
            return sfi.hIcon;
        }

        return nullptr;
    }

    GLuint BitmapToTexture(HBITMAP bitmap)
    {
        if (!bitmap) return 0;

        BITMAP bm{};
        if (!GetObject(bitmap, sizeof(bm), &bm))
            return 0;

        BITMAPINFO bmpInfo{};
        bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmpInfo.bmiHeader.biWidth = bm.bmWidth;
        bmpInfo.bmiHeader.biHeight = -bm.bmHeight;
        bmpInfo.bmiHeader.biPlanes = 1;
        bmpInfo.bmiHeader.biBitCount = 32;
        bmpInfo.bmiHeader.biCompression = BI_RGB;

        std::vector<BYTE> pixels(bm.bmWidth * bm.bmHeight * 4);
        HDC dc = GetDC(nullptr);
        int rows = GetDIBits(dc, bitmap, 0, bm.bmHeight, pixels.data(), &bmpInfo, DIB_RGB_COLORS);
        ReleaseDC(nullptr, dc);
        if (rows == 0)
            return 0;

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            bm.bmWidth, bm.bmHeight, 0,
            GL_BGRA, GL_UNSIGNED_BYTE,
            pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        DeleteObject(bitmap);
        return tex;

    }

    GLuint IconToTexture(HICON icon)
    {
        if (!icon) return 0;

        ICONINFO iconInfo;
        if (!GetIconInfo(icon, &iconInfo)) return 0;

        return BitmapToTexture(iconInfo.hbmColor);
    }

    void OpenGLRenderer::MeshGPUHandle::GenVertexBuffer(const Mesh& mesh)
    {
        glGenBuffers(1, &mVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.vertices.size(), mesh.vertices.data(), GL_STATIC_DRAW);
    }

    void OpenGLRenderer::MeshGPUHandle::GenIndexBuffer(const Mesh& mesh)
    {
        glGenBuffers(1, &mIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * mesh.indices.size(), mesh.indices.data(), GL_STATIC_DRAW);

        mIndexCount = mesh.indices.size();
    }

    void OpenGLRenderer::MeshGPUHandle::BindBuffers()
    {
        glGenVertexArrays(1, &mVertexArrayObject);
        glBindVertexArray(mVertexArrayObject);

        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);

        glBindVertexArray(0);
    }

    void OpenGLRenderer::MeshGPUHandle::DeleteBuffers()
    {
        glDeleteBuffers(1, &mIndexBuffer);
        glDeleteBuffers(1, &mVertexBuffer);
        glDeleteVertexArrays(1, &mVertexArrayObject);

#ifdef _DEBUG
        mVertexArrayObject = num_max<GLuint>();
        mVertexBuffer = num_max<GLuint>();
        mIndexBuffer = num_max<GLuint>();
#endif // _DEBUG
    }
}
