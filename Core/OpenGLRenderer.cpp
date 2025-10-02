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
#include "Conversion.h"

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
        if (mModels.contains(path))
        {
            Gep::Log::Error("Cannot load mesh: [", path, "] a mesh with that name has already been loaded");
            return;
        }

        auto& [modelHandle, model] = mModels[path];

        model = LoadModelFromFile(path);

        for (const auto& mesh : model.meshes)
        {
            MeshGPUHandle& meshHandle = modelHandle.meshHandles.emplace_back(); // create a handle for this mesh

            const Material& material = model.materials.at(mesh.materialIndex);

            meshHandle.materialHandle.diffuseTexture   = material.diffuseTextureHandle;
            meshHandle.materialHandle.aoTexture        = material.aoTextureHandle;
            meshHandle.materialHandle.metalnessTexture = material.metalnessTextureHandle;
            meshHandle.materialHandle.roughnessTexture = material.roughnessTextureHandle;
            
            meshHandle.GenVertexBuffer(mesh);
            meshHandle.GenIndexBuffer(mesh);
            meshHandle.BindBuffers();
        }
    }

    void OpenGLRenderer::AddModel(const std::string& name, const Gep::Model& newModel)
    {
        if (mModels.contains(name))
        {
            Gep::Log::Error("Cannot load mesh: [", name, "] a mesh with that name has already been loaded");
            return;
        }

        auto [it, inserted] = mModels.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(ModelGPUHandle{}, newModel)
        );

        auto& [modelHandle, model] = it->second;

        for (const Mesh& mesh : model.meshes)
        {
            MeshGPUHandle& meshHandle = modelHandle.meshHandles.emplace_back();

            meshHandle.materialHandle.diffuseTexture = GetErrorTexture();
            meshHandle.GenVertexBuffer(mesh);
            meshHandle.GenIndexBuffer(mesh);
            meshHandle.BindBuffers();
        }
    }

    void OpenGLRenderer::AddAnimation(const std::string& name, const Gep::Animation& animation)
    {
        if (mAnimations.contains(name))
        {
            Gep::Log::Error("Adding animation failed, animation with the name: [", name, "] was already loaded");
            return;
        }

        mAnimations[name] = std::make_pair(AnimationGPUHandle{}, animation);
    }

    const Gep::Model& OpenGLRenderer::GetModel(const std::string& name)
    {
        if (!mModels.contains(name))
        {
            Gep::Log::Critical("Attempting to get a model with name: [", name, "] that doesn't exist");
        }

        return mModels.at(name).second;
    }

    const Gep::Animation& OpenGLRenderer::GetAnimation(const std::string& name)
    {
        if (!mAnimations.contains(name))
        {
            Gep::Log::Critical("Attempting to get a animation with name: [", name, "] that doesn't exist");
        }

        return mAnimations.at(name).second;
    }

    bool OpenGLRenderer::IsAnimationLoaded(const std::string& name)
    {
        return mAnimations.contains(name);
    }

    bool OpenGLRenderer::IsMeshLoaded(const std::string& name) const
    {
        return mModels.contains(name);
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

    void OpenGLRenderer::SetLineShader(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath)
    {
        mLineShader = std::make_unique<Shader>(Shader::FromFile(vertPath, fragPath));
    }

    void OpenGLRenderer::AddObject(const std::string& modelName, const ObjectGPUData& objectData)
    {
        mModels.at(modelName).first.objectDatas.push_back(objectData);
    }

    void OpenGLRenderer::AddCamera(const CameraGPUData& uniforms)
    {
        mCameraUniforms.push_back(uniforms);
    }

    void OpenGLRenderer::AddLight(const LightGPUData& uniforms)
    {
        mLightUniforms.push_back(uniforms);
    }

    void OpenGLRenderer::AddLine(const LineGPUData& lines)
    {
        mLineUniforms.push_back(lines);
    }

    void OpenGLRenderer::CommitObjects()
    {
        for (auto& [modelName, modelPair] : mModels)
        {
            auto& [modelHandle, model] = modelPair;

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
        mLineShader->SetUniform(0, static_cast<int>(index));
    }

    void OpenGLRenderer::SetLightCount(size_t count)
    {
        mPBRShader->SetUniform(2, static_cast<int>(count));
        mHighlightShader->SetUniform(2, static_cast<int>(count));
    }

    void OpenGLRenderer::UnloadModel(const std::string& name)
    {
        if (!mModels.contains(name))
        {
            Gep::Log::Error("Cannot unload mesh: [", name, "] a mesh with that name has not been loaded");
            return;
        }

        // aquire the model id from the name
        auto& [modelHandle, model] = mModels[name];

        // delete all meshes owned by the model
        for (MeshGPUHandle& meshHandle : modelHandle.meshHandles)
        {
            meshHandle.DeleteBuffers();
        }

        mModels.erase(name);
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

    std::vector<std::string> OpenGLRenderer::GetLoadedMeshes() const
    {
        std::vector<std::string> meshes;

        for (const auto& [name, modelHandle] : mModels)
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

    std::vector<std::string> OpenGLRenderer::GetLoadedAnimations() const
    {
        std::vector<std::string> animations;

        for (const auto& [name, pair] : mAnimations)
        {
            animations.push_back(name);
        }

        return animations;
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

        if (mTextures.contains(texturePath.string())) {
            Gep::Log::Error("Cannot load texture: [", texturePath, "] has already been loaded");
            return;
        }

        if (!std::filesystem::exists(texturePath)) {
            Gep::Log::Error("Cannot load texture: [", texturePath.string(), "] does not exist");
            return;
        }

        mTextures[texturePath.string()] = GetErrorTexture();

        std::thread([&]()
            {
                LoadTexture(texturePath);
            }).detach();
    }

    void OpenGLRenderer::LoadTexture(const std::filesystem::path& texturePath)
    {
        if (mTextures.contains(texturePath.string()))
        {
            Gep::Log::Error("Cannot load texture: [", texturePath.string(), "] is already loaded");
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

        LoadTextureFromPixelData(texturePath.string(), image, width, height, required_channels);
        stbi_image_free(image);
    }

    void OpenGLRenderer::LoadTexture(const std::string& name, const uint8_t* imageFileData, size_t size)
    {
        if (mTextures.contains(name))
        {
            Gep::Log::Error("Cannot load texture: [", name, "] is already loaded");
            return;
        }

        int requiredChannels = 4; // Force RGBA
        int width, height, channels;
        unsigned char* image = stbi_load_from_memory(imageFileData, size, &width, & height, & channels, requiredChannels);
        if (!image)
        {
            Gep::Log::Error("Failed to load texture from raw data, with the given name, [", name, "]");
            return;
        }

        LoadTextureFromPixelData(name, image, width, height, requiredChannels);
        stbi_image_free(image);
    }

    void OpenGLRenderer::LoadTextureFromPixelData(const std::string& name, const uint8_t* pixelData, size_t width, size_t height, int requiredChannels)
    {
        if (mTextures.contains(name))
        {
            Gep::Log::Error("Cannot load texture: [", name, "] is already loaded");
            return;
        }

        GLuint& texture = mTextures[name];
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Ensure proper alignment
        GLenum format = (requiredChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixelData);

        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
    }

    GLuint OpenGLRenderer::GetTexture(const std::string& textureName)
    {
        if (!mTextures.contains(textureName))
        {
            Gep::Log::Error("Cannot get texture: [", textureName, "] a texture with that name has not been loaded");
            return GetErrorTexture();
        }

        return mTextures.at(textureName);
    }

    GLuint OpenGLRenderer::GetOrLoadTexture(const std::filesystem::path& texturePath)
    {
        if (!mTextures.contains(texturePath.string()))
            LoadTexture(texturePath);

        if (!mTextures.contains(texturePath.string()))
            return GetErrorTexture();

        return mTextures.at(texturePath.string());
    }

    void OpenGLRenderer::LoadErrorTexture(const std::filesystem::path& texturePath)
    {
        LoadTexture(texturePath);
        mErrorTexture = GetTexture(texturePath.string());
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
        DrawLines();
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

    void OpenGLRenderer::SetUpLineDrawing()
    {
        glGenVertexArrays(1, &mLineVAO);
        glGenBuffers(1, &mLineVBO);

        glBindVertexArray(mLineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mLineVBO);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
    }

    void OpenGLRenderer::DrawRegular()
    {
        mPBRShader->Bind();

        size_t baseInstance = 0;
        for (auto& [modelName, modelPair] : mModels)
        {
            auto& [modelHandle, model] = modelPair;

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
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            for (const MeshGPUHandle& meshHandle : modelHandle.meshHandles)
            {
                glBindVertexArray(meshHandle.mVertexArrayObject);
                glDrawElementsInstancedBaseInstance(
                    GL_TRIANGLES,
                    meshHandle.mIndexCount,
                    GL_UNSIGNED_INT,
                    0,
                    modelHandle.wireframeObjectDatas.size(),
                    baseInstance
                );
            }
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            baseInstance += modelHandle.wireframeObjectDatas.size();

            modelHandle.objectDatas.clear();
            modelHandle.regularObjectDatas.clear();
            modelHandle.wireframeObjectDatas.clear();
        }

        mPBRShader->Unbind();
    }

    void OpenGLRenderer::DrawLines()
    {
        glDisable(GL_DEPTH_TEST);
        mLineShader->Bind();
        glBindVertexArray(mLineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mLineVBO);

        for (const LineGPUData& lineData : mLineUniforms)
        {
            // one color per set
            mLineShader->SetUniform(1, glm::vec4(lineData.color, 1.0f));

            glBufferData(GL_ARRAY_BUFFER,
                lineData.points.size() * sizeof(glm::vec3) * 2,
                lineData.points.data(),
                GL_STREAM_DRAW
            );

            // draw all line segments in this set
            glDrawArrays(GL_LINES, 0, lineData.points.size() * 2);
        }

        mLineShader->Unbind();
        mLineUniforms.clear();
        glEnable(GL_DEPTH_TEST);
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

    struct BoneInfo
    {
        int id;
        glm::mat4 offset;
    };

    static std::unordered_map<std::string, BoneInfo> gBoneInfoMap; //
    static std::unordered_map<std::string, VQS> gBoneData;
    static int gBoneCounter = 0;

    GLuint OpenGLRenderer::LoadMaterial(const std::filesystem::path& modelPath, const aiMaterial* assimpMaterial, const aiScene* scene, const aiTextureType type)
    {
        auto root = modelPath.parent_path();

        aiString texPath;
        if (aiReturn_SUCCESS == assimpMaterial->GetTexture(type, 0, &texPath))
        {
            if (texPath.C_Str()[0] == '*') // if the first character is a star it is embedded
            {
                int assimpTextureIndex = std::atoi(texPath.C_Str() + 1);
                aiTexture* assimpTexture = scene->mTextures[assimpTextureIndex];
                std::string textureName = modelPath.string() + "_EMBEDDED_" + std::to_string(assimpTextureIndex);

                if (assimpTexture->mHeight == 0) // if no height then it is compressed
                {
                    std::vector<uint8_t> bytes(
                        reinterpret_cast<uint8_t*>(assimpTexture->pcData),
                        reinterpret_cast<uint8_t*>(assimpTexture->pcData) + assimpTexture->mWidth
                    );

                    LoadTexture(textureName, bytes.data(), bytes.size());
                }
                else
                {
                    const int assimpTextureChannels = 4;
                    LoadTextureFromPixelData(textureName, reinterpret_cast<uint8_t*>(assimpTexture->pcData), assimpTexture->mWidth, assimpTexture->mHeight, assimpTextureChannels);
                }

                return GetTexture(textureName);
            }
            else
            {
                LoadTexture(root / texPath.C_Str());

                return GetTexture((root / texPath.C_Str()).string());
            }
        }

        return num_max<GLuint>();
    }

    void OpenGLRenderer::LoadAnimation(const aiAnimation* assimpAnimation, const Skeleton& skeleton)
    {
        const auto& [it, inserted] = mAnimations.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(assimpAnimation->mName.C_Str()),
            std::forward_as_tuple()
        );
        auto& [name, pair] = *it;
        auto& [animationHandle, animation] = pair;

        animation.duration = static_cast<float>(assimpAnimation->mDuration);
        animation.ticksPerSecond = assimpAnimation->mTicksPerSecond != 0.0
            ? static_cast<float>(assimpAnimation->mTicksPerSecond)
            : 25.0f; // Assimp default

        animation.tracks.reserve(assimpAnimation->mNumChannels);

        for (unsigned int i = 0; i < assimpAnimation->mNumChannels; i++)
        {
            const aiNodeAnim* channel = assimpAnimation->mChannels[i];

            // find bone index in skeleton
            auto it = std::find_if(skeleton.bones.begin(), skeleton.bones.end(), [&](const Bone& b)
            { 
                return b.name == channel->mNodeName.C_Str(); 
            });

            if (it == skeleton.bones.end())
                continue; // channel for a node that's not a bone

            uint16_t boneIndex = static_cast<uint16_t>(std::distance(skeleton.bones.begin(), it));

            Track& track = animation.tracks.emplace_back();
            track.boneIndex = boneIndex;

            // merge Assimp’s position/rotation/scale keys into VQS keyframes
            size_t numKeys = std::max({ channel->mNumPositionKeys,
                                        channel->mNumRotationKeys,
                                        channel->mNumScalingKeys });

            track.keyFrames.reserve(numKeys);
            for (size_t k = 0; k < numKeys; k++)
            {
                KeyFrame& frame = track.keyFrames.emplace_back();
                frame.time = 0.0f;
                frame.transform.position = glm::vec3(0.0f);
                frame.transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                frame.transform.scale = glm::vec3(1.0f);

                // pick closest available key for each channel
                if (k < channel->mNumPositionKeys)
                {
                    frame.time = static_cast<float>(channel->mPositionKeys[k].mTime);
                    frame.transform.position = ToVec3(channel->mPositionKeys[k].mValue);
                }

                if (k < channel->mNumRotationKeys)
                {
                    frame.time = static_cast<float>(channel->mRotationKeys[k].mTime);
                    frame.transform.rotation = ToQuat(channel->mRotationKeys[k].mValue);
                    frame.transform.rotation = glm::normalize(frame.transform.rotation);
                }

                if (k < channel->mNumScalingKeys)
                {
                    frame.time = static_cast<float>(channel->mScalingKeys[k].mTime);
                    frame.transform.scale = ToVec3(channel->mScalingKeys[k].mValue);
                }
            }
        }
    }

    Gep::VQS OpenGLRenderer::Interpolate(const Track & track, float time)
    {
        if (track.keyFrames.empty())
        {
            return VQS{}; // identity
        }

        // if time is before the first key
        if (time <= track.keyFrames.front().time || track.keyFrames.size() == 1)
        {
            return track.keyFrames.front().transform;
        }

        // if time is after the last key
        if (time >= track.keyFrames.back().time)
        {
            return track.keyFrames.back().transform;
        }

        // find the two keys around `time`
        size_t i = 0;
        while (i < track.keyFrames.size() - 1 && time > track.keyFrames[i + 1].time)
            i++;

        const KeyFrame& k1 = track.keyFrames[i];
        const KeyFrame& k2 = track.keyFrames[i + 1];

        float factor = (time - k1.time) / (k2.time - k1.time);

        Gep::VQS result;
        result.position = glm::lerp(k1.transform.position, k2.transform.position, factor);
        result.rotation = glm::slerp(k1.transform.rotation, k2.transform.rotation, factor);
        result.scale    = glm::lerp(k1.transform.scale,    k2.transform.scale,    factor);
        return result;
    }

    // moves all data from the aiScene into the internal model format
    void OpenGLRenderer::LoadMaterials(Gep::Model& model, const std::filesystem::path& path, const aiScene* scene)
    {
        model.materials.reserve(scene->mNumMaterials);

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            Material& material = model.materials.emplace_back();
            const aiMaterial* assimpMaterial = scene->mMaterials[i];


            aiColor3D diffuseColor(1.f, 1.f, 1.f);
            if (aiReturn_SUCCESS == assimpMaterial->Get("$clr.diffuse", 0, 0, diffuseColor))
                material.color = { diffuseColor.r, diffuseColor.g, diffuseColor.b };

            material.diffuseTextureHandle = LoadMaterial(path, assimpMaterial, scene, aiTextureType_DIFFUSE);
            material.hasDiffuseTexture    = (material.diffuseTextureHandle != num_max<GLuint>());

            material.aoTextureHandle = LoadMaterial(path, assimpMaterial, scene, aiTextureType_AMBIENT_OCCLUSION);
            material.hasAoTexture    = (material.aoTextureHandle != num_max<GLuint>());

            material.metalnessTextureHandle = LoadMaterial(path, assimpMaterial, scene, aiTextureType_METALNESS);
            material.hasMetalnessTexture = (material.metalnessTextureHandle != num_max<GLuint>());

            material.roughnessTextureHandle = LoadMaterial(path, assimpMaterial, scene, aiTextureType_DIFFUSE_ROUGHNESS);
            material.hasRoughnessTexture = (material.roughnessTextureHandle != num_max<GLuint>());
        }
    }

    void OpenGLRenderer::LoadAnimations(Gep::Model& model, const aiScene* scene)
    {
        for (uint32_t i = 0; i < scene->mNumAnimations; ++i)
        {
            LoadAnimation(scene->mAnimations[i], model.skeleton);
        }
    }

    // vertex bone datas are flaged with a max int if not set
    static void SetVertexBoneData(Vertex& v, int boneID, float weight)
    {
        for (int i = 0; i < v.boneWeights.size(); ++i)
        {
            if (v.boneIndices[i] == num_max<uint64_t>())
            {
                v.boneWeights[i] = weight;
                v.boneIndices[i] = boneID;
                break;
            }
        }
    }

    static void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, const aiMesh* assimpMesh)
    {
        for (aiBone* bone : std::span(assimpMesh->mBones, assimpMesh->mNumBones))
        {
            const std::string boneName = bone->mName.C_Str();
            int boneID = -1;

            if (!gBoneInfoMap.contains(boneName))
            {
                BoneInfo newBoneInfo{};
                newBoneInfo.id = gBoneCounter;
                newBoneInfo.offset = ToMat4(bone->mOffsetMatrix);
                gBoneInfoMap[boneName] = newBoneInfo;
                boneID = gBoneCounter;
                ++gBoneCounter;
            }
            else
            {
                boneID = gBoneInfoMap[boneName].id;
            }

            for (aiVertexWeight weight : std::span(bone->mWeights, bone->mNumWeights))
            {
                Vertex& v = vertices[weight.mVertexId];
                
                SetVertexBoneData(v, boneID, weight.mWeight);
            }
        }
    }

    static void LoadVertices(Gep::Mesh& mesh, const aiMesh* assimpMesh)
    {
        mesh.vertices.reserve(assimpMesh->mNumVertices);

        for (unsigned int i = 0; i < assimpMesh->mNumVertices; ++i)
        {
            Vertex& v = mesh.vertices.emplace_back();

            v.position = { assimpMesh->mVertices[i].x, assimpMesh->mVertices[i].y, assimpMesh->mVertices[i].z };

            if (assimpMesh->HasNormals())
                v.normal = { assimpMesh->mNormals[i].x, assimpMesh->mNormals[i].y, assimpMesh->mNormals[i].z };

            if (assimpMesh->HasTextureCoords(0))
                v.texCoord = { assimpMesh->mTextureCoords[0][i].x, assimpMesh->mTextureCoords[0][i].y };
        }

        ExtractBoneWeightForVertices(mesh.vertices, assimpMesh);
    }

    static void LoadIndices(Gep::Mesh& mesh, const aiMesh* assimpMesh)
    {
        for (unsigned int i = 0; i < assimpMesh->mNumFaces; ++i)
        {
            const aiFace& face = assimpMesh->mFaces[i];

            for (unsigned int j = 0; j < face.mNumIndices; ++j)
                mesh.indices.push_back(face.mIndices[j]);
        }
    }

    // returns the index of the node just created
    static size_t LoadHierarchyStep(Gep::Model& model, size_t parentIndex, const aiNode* node, const VQS& accumulatedTransform = VQS{})
    {
        if (!node) return num_max<size_t>();

        // convert current node's transformation to VQS and combine with accumulated transform
        VQS currentTransform = ToVQS(node->mTransformation);
        VQS combinedTransform = accumulatedTransform * currentTransform;

        auto it = gBoneData.find(node->mName.C_Str());

        // if node is not a bone, skip adding it but accumulate its transform
        if (it == gBoneData.end())
        {
            // but still traverse children, passing down the accumulated transform
            size_t lastValid = num_max<size_t>();
            for (size_t i = 0; i < node->mNumChildren; ++i)
            {
                lastValid = LoadHierarchyStep(model, parentIndex, node->mChildren[i], combinedTransform);
            }

            return lastValid;
        }

        const auto& [boneName, inverseBind] = *it;

        // this is a bone, so add it, note cannot get a reference here because it could be stale
        size_t index = model.skeleton.bones.size();
        model.skeleton.bones.emplace_back();
        model.skeleton.bones.at(index).name = node->mName.C_Str();
        model.skeleton.bones.at(index).parentIndex = parentIndex;
        model.skeleton.bones.at(index).transformation = combinedTransform; // use combined transform
        model.skeleton.bones.at(index).inverseBind = inverseBind;

        for (size_t i = 0; i < node->mNumChildren; ++i)
        {
            // reset accumulated transform for children since this bone will handle the transform hierarchy
            size_t childIndex = LoadHierarchyStep(model, index, node->mChildren[i]);
            if (childIndex != num_max<size_t>())
                model.skeleton.bones.at(index).childrenIndices.push_back(childIndex);
        }

        return index;
    }

    // create hierary
    static void LoadHierarchy(Gep::Model& model, const aiScene* scene)
    {
        LoadHierarchyStep(model, num_max<size_t>(), scene->mRootNode);
    }

    static void LoadMeshes(Gep::Model& model, const aiScene* scene)
    {
        model.meshes.reserve(scene->mNumMeshes);

        for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        {
            Mesh& mesh = model.meshes.emplace_back();

            LoadVertices(mesh, scene->mMeshes[i]);
            LoadIndices(mesh, scene->mMeshes[i]);

            mesh.materialIndex = scene->mMeshes[i]->mMaterialIndex;
        }
    }

    static void LoadBoneData(const aiScene* scene)
    {
        for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
        {
            aiMesh* mesh = scene->mMeshes[m];
            for (unsigned int b = 0; b < mesh->mNumBones; ++b)
            {
                aiBone* bone = mesh->mBones[b];
                gBoneData[bone->mName.C_Str()] = ToVQS(bone->mOffsetMatrix);
            }
        }
    }

    Model OpenGLRenderer::LoadModelFromFile(const std::filesystem::path& path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(),
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_SortByPType |
            aiProcess_OptimizeGraph |
            aiProcess_OptimizeMeshes 
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            Gep::Log::Error("Assimp error: ", importer.GetErrorString());
            return {};
        }

        Gep::Model model;



        LoadBoneData(scene);
        LoadMeshes(model, scene);
        LoadMaterials(model, path, scene);
        LoadHierarchy(model, scene);
        LoadAnimations(model, scene);


        return model;
    }
}
