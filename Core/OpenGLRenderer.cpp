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
#include "Algorithms.hpp"
#include "VQS.hpp"

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
    static Texture IconToTexture(HICON icon);
    static Texture BitmapToTexture(HBITMAP bitmap);

    void OpenGLRenderer::AddModelFromFile(const std::string& path)
    {
        if (mModels.contains(path))
        {
            Gep::Log::Error("Cannot load mesh: [", path, "] a mesh with that name has already been loaded");
            return;
        }

        auto& [modelHandle, model] = mModels[path];

        model = LoadModelFromFile(path);

        // moves all of the model data from the cpu onto the gpu, and keeps track of the handles
        for (const auto& mesh : model.meshes)
        {
            MeshGPUHandle& meshHandle = modelHandle.meshHandles.emplace_back(); // create a handle for this mesh

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

    void OpenGLRenderer::AddMaterial(const Gep::Material& material)
    {
        mMaterials.insert(material);
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

    bool OpenGLRenderer::IsAnimationLoaded(const std::string& name) const
    {
        return mAnimations.contains(name);
    }

    bool OpenGLRenderer::IsModelLoaded(const std::string& name) const
    {
        return mModels.contains(name);
    }

    bool OpenGLRenderer::IsShaderLoaded(const std::string& name) const
    {
        return mShaders.contains(name);
    }

    void OpenGLRenderer::AddObject(const std::string& shaderName, const std::string& modelName, const ObjectGPUData& gpuData, RenderFlags flags)
    {
        // these existance checks are very expensive so only perform in debug mode
        debug_if (!IsModelLoaded(modelName))
        {
            Gep::Log::Error("Failed to draw object. The model: [", modelName, "] doesn't exist");
            return;
        }

        debug_if (!IsShaderLoaded(shaderName))
        {
            Gep::Log::Error("Failed to draw object. The shader: [", shaderName, "] doesn't exist");
            return;
        }

        mObjectDatas[shaderName][modelName][flags].push_back(gpuData);
    }

    void OpenGLRenderer::AddCamera(const CameraGPUData& uniforms)
    {
        mCameraUniforms.push_back(uniforms);
    }

    void OpenGLRenderer::AddPointLight(const PointLightGPUData& uniforms)
    {
        mPointLightUniforms.push_back(uniforms);
    }

    void OpenGLRenderer::AddDirectionalLight(const DirectionalLightGPUData& uniforms)
    {
        mDirectionalLightUniforms.push_back(uniforms);
    }

    void OpenGLRenderer::AddBone(const BoneGPUData& boneData)
    {
        mBoneUniforms.push_back(boneData);
    }

    void OpenGLRenderer::AddLine(const LineGPUData& lines)
    {
        mLineUniforms.push_back(lines);
    }

    void OpenGLRenderer::CommitObjects()
    {
        static std::unordered_map<uint32_t, uint32_t> materialMapping;
        materialMapping.clear();

        for (const auto& [id, material] : mMaterials)
        {
            size_t uniformIndex = mMaterialUniforms.size();
            MaterialGPUData& gpuMaterial = mMaterialUniforms.emplace_back();

            // ambient occlusion
            gpuMaterial.ao = material.ao;
            gpuMaterial.aoTextureHandle = material.aoTexture.handle;

            // color
            gpuMaterial.color = material.color;
            gpuMaterial.colorTextureHandle = material.diffuseTexture.handle;

            // metalness
            gpuMaterial.metalness = material.metalness;
            gpuMaterial.metalnessTextureHandle = material.metalnessTexture.handle;

            // roughness
            gpuMaterial.roughness = material.roughness;
            gpuMaterial.roughnessTextureHandle = material.roughnessTexture.handle;

            // normal map
            gpuMaterial.normalTextureHandle = material.normalTexture.handle;

            // NOTE: These debug overrides force materials
            //gpuMaterial.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            //gpuMaterial.ao = 0.8f;
            //gpuMaterial.metalness = 0.8f;
            //gpuMaterial.roughness = 0.8f;

            materialMapping[id] = static_cast<uint32_t>(uniformIndex);
        }

        // 1: loops over each shader
        for (const auto& [shaderName, modelToFlags] : mObjectDatas)
        {
            // 2: loops over each model using the current shader
            for (const auto& [modelName, flagsToObjects] : modelToFlags)
            {
                const Gep::Model& model = GetModel(modelName);

                // 3: loops over each active flag bucket
                for (const auto& [flags, objects] : flagsToObjects)
                {
                    // add all per-object instance data
                    mObjectUniforms.insert(mObjectUniforms.end(), objects.begin(), objects.end());

                    // Pack mMeshUniforms in the same order DrawRegular consumes:
                    // per-mesh, then per-instance.
                    for (const auto& mesh : model.meshes)
                    {
                        const uint32_t matIdx = materialMapping.at(mesh.materialIndex);
                        for (size_t i = 0; i < objects.size(); ++i)
                        {
                            mMeshUniforms.push_back({
                                .materialIndex = matIdx
                            });
                        }
                    }
                }
            }
        }

        mObjectUniforms.commit();

        // send all per mesh/material data to the gpu
        CommitMeshes();
        CommitMaterials();
    }
    void OpenGLRenderer::CommitCameras()
    {
        mCameraUniforms.commit();
    }

    void OpenGLRenderer::CommitBones()
    {
        mBoneUniforms.commit();
    }

    void OpenGLRenderer::CommitLights()
    {
        SetPointLightCount(mPointLightUniforms.size());
        mPointLightUniforms.commit();

        SetDirectionalLightCount(mDirectionalLightUniforms.size());
        mDirectionalLightUniforms.commit();
    }

    void OpenGLRenderer::CommitMeshes()
    {
        mMeshUniforms.commit();
    }

    void OpenGLRenderer::CommitMaterials()
    {
        mMaterialUniforms.commit();
    }

    void OpenGLRenderer::SetCameraIndex(uint32_t index)
    {
        for (const auto& [shaderName, shader] : mShaders)
        {
            shader->SetUniform(0, index);
        }
    }

    void OpenGLRenderer::SetPointLightCount(uint32_t count)
    {
        for (const auto& [shaderName, shader] : mShaders)
        {
            shader->SetUniform(2, count);
        }
    }

    void OpenGLRenderer::SetDirectionalLightCount(uint32_t count)
    {
        for (const auto& [shaderName, shader] : mShaders)
        {
            shader->SetUniform(4, count);
        }
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

    void OpenGLRenderer::Start(const glm::vec3& color)
    {
        //glClearColor(color.r, color.g, color.b, 1);
        //glClearDepth(1);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    std::vector<std::string> OpenGLRenderer::GetLoadedModels() const
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

        Texture texture = IconToTexture(icon);
        if (texture.id == 0)
        {
            Gep::Log::Error("Failed to convert icon to texture: [", iconPath.string(), "]");
            return;
        }

        mIconTextures[iconPath.extension().string()] = texture;
    }

    Texture OpenGLRenderer::GetIconTexture(const std::string& extension)
    {
        if (!mIconTextures.contains(extension))
        {
            Gep::Log::Error("Cannot get icon texture: [", extension, "] an icon with that extension has not been loaded");
            return {};
        }

        return mIconTextures.at(extension);
    }

    Texture OpenGLRenderer::GetOrLoadIconTexture(const std::filesystem::path& iconPath)
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

    void OpenGLRenderer::LoadShader(const std::string& name, const std::filesystem::path& vert, const std::filesystem::path& frag)
    {
        mShaders.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(std::make_unique<Shader>(Shader::FromFile(vert, frag)))
        );
    }

    void OpenGLRenderer::ReloadShaders()
    {
        for (auto& [shaderName, shader] : mShaders)
        {
            shader->Reload();
        }
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
        unsigned char* image = stbi_load_from_memory(imageFileData, size, &width, &height, &channels, requiredChannels);
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

        Texture& texture = mTextures[name];
        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Ensure proper alignment
        GLenum format = (requiredChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixelData);

        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        texture.handle = glGetTextureHandleARB(texture.id);
        glMakeTextureHandleResidentARB(texture.handle);

        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
    }

    Texture OpenGLRenderer::GetTexture(const std::string& textureName)
    {
        if (!mTextures.contains(textureName))
        {
            Gep::Log::Error("Cannot get texture: [", textureName, "] a texture with that name has not been loaded");
            return GetErrorTexture();
        }

        return mTextures.at(textureName);
    }

    Texture OpenGLRenderer::GetOrLoadTexture(const std::filesystem::path& texturePath)
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

    Texture OpenGLRenderer::GetErrorTexture() const
    {
        if (mErrorTexture.id == 0)
        {
            Gep::Log::Error("Cannot get error texture: an error texture has not been loaded");
            return {};
        }

        return mErrorTexture;
    }

    void OpenGLRenderer::Draw() const
    {
        DrawRegular();
        DrawLines();
    }

    void OpenGLRenderer::End()
    {
        mLineUniforms.clear();
        mObjectDatas.clear();

        mPointLightUniforms.clear();
        mDirectionalLightUniforms.clear();
        mObjectUniforms.clear();
        mCameraUniforms.clear();
        mBoneUniforms.clear();
        mMeshUniforms.clear();
        mMaterialUniforms.clear();
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

    void OpenGLRenderer::DrawGeometry() const
    {
        mGeometryFrameBuffer.Bind();
        mGeometryFrameBuffer.UpdateViewport();
        mGeometryFrameBuffer.Clear();

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        uint32_t baseInstance = 0;
        uint32_t meshBaseInstance = 0;

        Shader& geometryShader = *mShaders.at("Geometry-Static");
        geometryShader.Bind();

        for (const auto& [shaderName, modelToFlags] : mObjectDatas)
        {
            for (const auto& [modelName, flagsToObjects] : modelToFlags)
            {
                const auto& [modelHandle, model] = mModels.at(modelName);

                for (const auto& [flags, objects] : flagsToObjects)
                {
                    if ((flags & RenderFlags::NoDepthTest) == RenderFlags::NoDepthTest)
                        glDisable(GL_DEPTH_TEST);
                    else
                        glEnable(GL_DEPTH_TEST);

                    if ((flags & RenderFlags::NoBackfaceCull) == RenderFlags::NoBackfaceCull)
                        glDisable(GL_CULL_FACE);
                    else
                    {
                        glEnable(GL_CULL_FACE);
                        glCullFace(GL_BACK);
                    }

                    for (const MeshGPUHandle& meshHandle : modelHandle.meshHandles)
                    {
                        glBindVertexArray(meshHandle.mVertexArrayObject);

                        geometryShader.SetUniform(3, meshBaseInstance);

                        glDrawElementsInstancedBaseInstance(
                            GL_TRIANGLES,
                            meshHandle.mIndexCount,
                            GL_UNSIGNED_INT,
                            0,
                            objects.size(),
                            baseInstance
                        );

                        meshBaseInstance += objects.size();
                    }

                    baseInstance += objects.size();
                }
            }
        }

        Shader::Unbind();
        mGeometryFrameBuffer.Unbind();
    }

    void OpenGLRenderer::DrawLighting() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        Shader& lightingShader = *mShaders.at("Lighting");
        lightingShader.Bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mGBuffer.position);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mGBuffer.normal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mGBuffer.albedo);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mGBuffer.metalRough);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, mGBuffer.ao);

        lightingShader.SetUniform("gPosition", 0);
        lightingShader.SetUniform("gNormal", 1);
        lightingShader.SetUniform("gAlbedo", 2);
        lightingShader.SetUniform("gMetalRough", 3);
        lightingShader.SetUniform("gAO", 4);

        DrawFullscreenTriangle();
    }

    void OpenGLRenderer::DrawRegular() const
    {
        uint32_t baseInstance = 0;
        uint32_t meshBaseInstance = 0;
        for (const auto& [shaderName, modelToFlags] : mObjectDatas)
        {
            Shader& currentShader = *mShaders.at(shaderName);

            currentShader.Bind();

            for (const auto& [modelName, flagsToObjects] : modelToFlags)
            {
                const auto& [modelHandle, model] = mModels.at(modelName);

                for (const auto& [flags, objects] : flagsToObjects)
                {
                    // Optional highlight pre-pass (outline)
                    if ((flags & RenderFlags::Highlight) == RenderFlags::Highlight)
                    {
                        if (mShaders.contains("Highlight"))
                        {
                            // Use dedicated highlight shader
                            Shader& outlineShader = *mShaders.at("Highlight");

                            if (outlineShader.IsValid())
                            {
                                outlineShader.Bind();

                                // Depth test behavior mirrors flag, but don't write depth for outline
                                if ((flags & RenderFlags::NoDepthTest) == RenderFlags::NoDepthTest)
                                    glDisable(GL_DEPTH_TEST);
                                else
                                    glEnable(GL_DEPTH_TEST);

                                glDepthMask(GL_FALSE);                // don't write outline into depth
                                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                                glEnable(GL_CULL_FACE);
                                glCullFace(GL_FRONT);                 // draw backfaces to create silhouette

                                for (const MeshGPUHandle& meshHandle : modelHandle.meshHandles)
                                {
                                    glBindVertexArray(meshHandle.mVertexArrayObject);

                                    // Maintain the same SSBO/UBO base consumption as the main pass
                                    outlineShader.SetUniform(3, meshBaseInstance);

                                    glDrawElementsInstancedBaseInstance(
                                        GL_TRIANGLES,
                                        meshHandle.mIndexCount,
                                        GL_UNSIGNED_INT,
                                        0,
                                        objects.size(),
                                        baseInstance
                                    );
                                }

                                // Restore default depth write and cull face mode
                                glDepthMask(GL_TRUE);
                                glCullFace(GL_BACK);

                                // Re-bind the main shader for the normal pass
                                currentShader.Bind();
                            }

                        }
                        else
                        {
                            Gep::Log::Error("No Highlight Shader Found!");
                        }
                    }

                    // Wireframe check (normal pass)
                    if ((flags & RenderFlags::Wireframe) == RenderFlags::Wireframe)
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    else
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                    // Depth test check (normal pass)
                    if ((flags & RenderFlags::NoDepthTest) == RenderFlags::NoDepthTest)
                        glDisable(GL_DEPTH_TEST);
                    else
                        glEnable(GL_DEPTH_TEST);

                    // Backface culling (normal pass)
                    if ((flags & RenderFlags::NoBackfaceCull) == RenderFlags::NoBackfaceCull)
                    {
                        glDisable(GL_CULL_FACE);
                    }
                    else
                    {
                        glEnable(GL_CULL_FACE);
                        glCullFace(GL_BACK);
                    }

                    // Draw meshes (normal pass)
                    for (const MeshGPUHandle& meshHandle : modelHandle.meshHandles)
                    {
                        glBindVertexArray(meshHandle.mVertexArrayObject);

                        currentShader.SetUniform(3, meshBaseInstance);

                        glDrawElementsInstancedBaseInstance(
                            GL_TRIANGLES,
                            meshHandle.mIndexCount,
                            GL_UNSIGNED_INT,
                            0,
                            objects.size(),
                            baseInstance
                        );

                        meshBaseInstance += objects.size();
                    }
                    baseInstance += objects.size();
                }
            }
        }

        Shader::Unbind();
    }
    void OpenGLRenderer::DrawLines() const
    {
        //glDisable(GL_DEPTH_TEST);
        Shader& lineShader = *mShaders.at("Line");
        lineShader.Bind();

        glBindVertexArray(mLineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mLineVBO);

        for (const LineGPUData& lineData : mLineUniforms)
        {
            // one color per set
            lineShader.SetUniform(1, glm::vec4(lineData.color, 1.0f));

            glBufferData(GL_ARRAY_BUFFER,
                lineData.points.size() * sizeof(glm::vec3) * 2,
                lineData.points.data(),
                GL_STREAM_DRAW
            );

            // draw all line segments in this set
            glDrawArrays(GL_LINES, 0, lineData.points.size() * 2);
        }

        lineShader.Unbind();
        //glEnable(GL_DEPTH_TEST);
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

    Texture BitmapToTexture(HBITMAP bitmap)
    {
        if (!bitmap) return {};

        BITMAP bm{};
        if (!GetObject(bitmap, sizeof(bm), &bm))
            return {};

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
            return {};

        Texture texture;
        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            bm.bmWidth, bm.bmHeight, 0,
            GL_BGRA, GL_UNSIGNED_BYTE,
            pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        texture.handle = glGetTextureHandleARB(texture.id);
        glMakeTextureHandleResidentARB(texture.handle);

        DeleteObject(bitmap);
        return texture;

    }

    Texture IconToTexture(HICON icon)
    {
        if (!icon) return {};

        ICONINFO iconInfo;
        if (!GetIconInfo(icon, &iconInfo)) return {};

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
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        glEnableVertexAttribArray(2);

        glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIndices));
        glEnableVertexAttribArray(3);

        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));
        glEnableVertexAttribArray(4);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);

        glBindVertexArray(0);
    }

    void OpenGLRenderer::MeshGPUHandle::DeleteBuffers()
    {
        glDeleteBuffers(1, &mIndexBuffer);
        glDeleteBuffers(1, &mVertexBuffer);
        glDeleteVertexArrays(1, &mVertexArrayObject);

#ifdef _DEBUG
        mVertexArrayObject = NumMax<GLuint>();
        mVertexBuffer = NumMax<GLuint>();
        mIndexBuffer = NumMax<GLuint>();
#endif // _DEBUG
    }

    struct BoneInfo
    {
        uint32_t index = 0;
        Gep::VQS offset{};
    };

    static std::unordered_map<std::string, BoneInfo> gBoneData;

    Texture OpenGLRenderer::LoadTexturesFromAssimpMaterial(const std::filesystem::path& modelPath, const aiMaterial* assimpMaterial, const aiScene* scene, const aiTextureType type)
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
                    // BGRA? format may cause issues remember this
                    Gep::Log::Critical("I'm not sure if this is ever used so this is going to crash if this is");
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

        return {};
    }

    void OpenGLRenderer::LoadAnimation(const std::string& parentPath, const aiAnimation* assimpAnimation, const Skeleton& skeleton)
    {
        const auto& [it, inserted] = mAnimations.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(std::string(assimpAnimation->mName.C_Str()) + " (" + parentPath + ")"),
            std::forward_as_tuple()
        );
        auto& [name, pair] = *it;
        auto& [animationHandle, animation] = pair;

        animation.duration = static_cast<float>(assimpAnimation->mDuration);
        animation.ticksPerSecond = assimpAnimation->mTicksPerSecond != 0.0
            ? static_cast<float>(assimpAnimation->mTicksPerSecond)
            : 25.0f; // Assimp default

        animation.tracks.reserve(assimpAnimation->mNumChannels);

        for (uint32_t i = 0; i < assimpAnimation->mNumChannels; i++)
        {
            const aiNodeAnim* channel = assimpAnimation->mChannels[i];

            // find bone index in skeleton
            auto it = std::find_if(skeleton.bones.begin(), skeleton.bones.end(), [&](const Bone& b)
            {
                return b.name == channel->mNodeName.C_Str();
            });

            if (it == skeleton.bones.end())
            {
                Gep::Log::Warning("Animation channel for bone '", channel->mNodeName.C_Str(), "' not found in skeleton");
                continue;
            }

            uint32_t boneIndex = static_cast<uint32_t>(std::distance(skeleton.bones.begin(), it));

            Track& track = animation.tracks.emplace_back();
            track.boneIndex = boneIndex;

            // loop through all keyframes reading in their time and position
            track.positionKeyFrames.reserve(channel->mNumPositionKeys);
            track.rotationKeyFrames.reserve(channel->mNumRotationKeys);
            track.scaleKeyFrames.reserve(channel->mNumScalingKeys);

            for (size_t k = 0; k < channel->mNumPositionKeys; k++)
            {
                auto& keyFrame = track.positionKeyFrames.emplace_back();

                keyFrame.time = static_cast<float>(channel->mPositionKeys[k].mTime);
                keyFrame.transform = ToVec3(channel->mPositionKeys[k].mValue);
            }
            for (size_t k = 0; k < channel->mNumRotationKeys; k++)
            {
                auto& keyFrame = track.rotationKeyFrames.emplace_back();

                keyFrame.time = static_cast<float>(channel->mRotationKeys[k].mTime);
                keyFrame.transform = glm::normalize(ToQuat(channel->mRotationKeys[k].mValue));
            }
            for (size_t k = 0; k < channel->mNumScalingKeys; k++)
            {
                auto& keyFrame = track.scaleKeyFrames.emplace_back();

                keyFrame.time = static_cast<float>(channel->mScalingKeys[k].mTime);
                keyFrame.transform = ToVec3(channel->mScalingKeys[k].mValue);
            }
        }
    }

    glm::quat OpenGLRenderer::InterpolateRotation(const Track& track, float time)
    {
        if (track.rotationKeyFrames.empty())
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity

        // if time is before the first key
        if (time <= track.rotationKeyFrames.front().time || track.rotationKeyFrames.size() == 1)
            return track.rotationKeyFrames.front().transform;

        // if time is after the last key
        if (time >= track.rotationKeyFrames.back().time)
            return track.rotationKeyFrames.back().transform;

        // find the two keys around `time`
        size_t i = 0;
        while (i < track.rotationKeyFrames.size() - 1 && time >= track.rotationKeyFrames[i + 1].time)
            i++;

        const auto& k1 = track.rotationKeyFrames[i];
        const auto& k2 = track.rotationKeyFrames[i + 1];

        float factor = (time - k1.time) / (k2.time - k1.time);

        return glm::slerp(k1.transform, k2.transform, factor);
    }

    glm::vec3 OpenGLRenderer::InterpolateScale(const Track& track, float time)
    {
        if (track.scaleKeyFrames.empty())
            return glm::vec3(1.0f); // identity

        // if time is before the first key
        if (time <= track.scaleKeyFrames.front().time || track.scaleKeyFrames.size() == 1)
            return track.scaleKeyFrames.front().transform;

        // if time is after the last key
        if (time >= track.scaleKeyFrames.back().time)
            return track.scaleKeyFrames.back().transform;
        
        // find the two keys around `time`
        size_t i = 0;
        while (i < track.scaleKeyFrames.size() - 1 && time >= track.scaleKeyFrames[i + 1].time)
            i++;

        const auto& k1 = track.scaleKeyFrames[i];
        const auto& k2 = track.scaleKeyFrames[i + 1];

        float factor = (time - k1.time) / (k2.time - k1.time);

        return glm::lerp(k1.transform, k2.transform, factor);
    }

    glm::vec3 OpenGLRenderer::InterpolatePosition(const Track& track, float time)
    {
        if (track.positionKeyFrames.empty())
            return glm::vec3{}; // identity

        // if time is before the first key
        if (time <= track.positionKeyFrames.front().time || track.positionKeyFrames.size() == 1)
            return track.positionKeyFrames.front().transform;

        // if time is after the last key
        if (time >= track.positionKeyFrames.back().time)
            return track.positionKeyFrames.back().transform;

        // find the two keys around `time`
        size_t i = 0;
        while (i < track.positionKeyFrames.size() - 1 && time >= track.positionKeyFrames[i + 1].time)
            i++;

        const auto& k1 = track.positionKeyFrames[i];
        const auto& k2 = track.positionKeyFrames[i + 1];

        float factor = (time - k1.time) / (k2.time - k1.time);

        return glm::lerp(k1.transform, k2.transform, factor);
    }

    // cleared once per call to load materials. used to map assimp material indexes to the internal mMaterials indexes
    static std::unordered_map<uint32_t, uint64_t> gAssimpMaterialIndexToMaterialIndex;

    // moves all data from the aiScene into the internal model format
    void OpenGLRenderer::LoadMaterials(const std::filesystem::path& path, const aiScene* scene)
    {
        gAssimpMaterialIndexToMaterialIndex.clear();

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            size_t materialIndex = mMaterials.emplace();
            Material& material = mMaterials.at(materialIndex);
            const aiMaterial* assimpMaterial = scene->mMaterials[i];

            gAssimpMaterialIndexToMaterialIndex[i] = materialIndex; // create the mapping

            aiColor3D outColor(1.f, 1.f, 1.f);
            if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, outColor))
                material.color = { outColor.r, outColor.g, outColor.b, 1.0f };

            if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, outColor))
                material.ao = outColor.r;

            if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, outColor))
                material.metalness = outColor.r;

            if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, outColor))
                material.roughness = outColor.r;

            material.diffuseTexture   = LoadTexturesFromAssimpMaterial(path, assimpMaterial, scene, aiTextureType_DIFFUSE);
            material.aoTexture        = LoadTexturesFromAssimpMaterial(path, assimpMaterial, scene, aiTextureType_AMBIENT_OCCLUSION);
            material.metalnessTexture = LoadTexturesFromAssimpMaterial(path, assimpMaterial, scene, aiTextureType_METALNESS);
            material.roughnessTexture = LoadTexturesFromAssimpMaterial(path, assimpMaterial, scene, aiTextureType_DIFFUSE_ROUGHNESS);
            material.normalTexture    = LoadTexturesFromAssimpMaterial(path, assimpMaterial, scene, aiTextureType_NORMALS);
        }
    }

    void OpenGLRenderer::LoadAnimations(const std::string& name, Gep::Model& model, const aiScene* scene)
    {
        for (uint32_t i = 0; i < scene->mNumAnimations; ++i)
        {
            LoadAnimation(name, scene->mAnimations[i], model.skeleton);
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
    static uint32_t LoadHierarchyStep(Gep::Model& model, const uint32_t parentIndex, const aiNode* node)
    {
        // if the passed node is null return num max signaling that this is a leaf
        if (!node) 
            return NumMax<uint32_t>();

        auto it = gBoneData.find(node->mName.C_Str());

        // if node is a bone sets it inverse bind otherwise leave as identity
        VQS inverseBind{};
        const bool isRealBone = it != gBoneData.end();
        if (isRealBone)
            inverseBind = it->second.offset;
        
        // create an entry in the heirarchy. 
        uint32_t index = model.skeleton.bones.size();
        Gep::Bone& bone = model.skeleton.bones.emplace_back();
        bone.name = node->mName.C_Str();
        bone.parentIndex = parentIndex;
        bone.transformation = ToVQS(node->mTransformation);
        bone.inverseBind = inverseBind;
        bone.isRealBone = isRealBone;

        // if its a bone add the index to the name association. Used when extracting vertex weights
        if (isRealBone) 
            it->second.index = index;

        // do the same thing for each child
        for (const aiNode* childNode : std::span(node->mChildren, node->mNumChildren))
        {
            uint32_t childIndex = LoadHierarchyStep(model, index, childNode);
            if (childIndex != NumMax<uint32_t>())
            {
                //note: cant get a reference here because it could be stale after recursive calls
                model.skeleton.bones.at(index).childrenIndices.push_back(childIndex);
            }
        }

        return index;
    }

    // create hierary
    static void LoadHierarchy(Gep::Model& model, const aiScene* scene)
    {
        // on the off chance a model doesn't have a root node?
        //uint32_t index = model.skeleton.bones.size();
        //Gep::Bone& bone = model.skeleton.bones.emplace_back();
        //bone.name = "Root";
        //bone.parentIndex = NumMax<uint32_t>();
        //bone.transformation = Gep::VQS{};
        //bone.inverseBind = Gep::VQS{};
        //bone.isRealBone = false;

        LoadHierarchyStep(model, NumMax<uint32_t>(), scene->mRootNode);
    }

    static void SetVertexBoneData(Vertex& vertex, uint32_t boneID, float weight)
    {
        for (int i = 0; i < vertex.boneIndices.size(); ++i)
        {
            if (vertex.boneIndices[i] == Vertex::INVALID_INDEX)
            {
                vertex.boneWeights[i] = weight;
                vertex.boneIndices[i] = boneID;
                break;
            }
        }
    }

    static void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, const aiMesh* assimpMesh, const aiScene* scene)
    {
        for (const aiBone* assimpBone : std::span(assimpMesh->mBones, assimpMesh->mNumBones))
        {
            const std::string boneName = assimpBone->mName.C_Str();
            const uint32_t boneID = gBoneData.at(boneName).index; // index into the final bone heirarchy

            for (const aiVertexWeight assimpWeight : std::span(assimpBone->mWeights, assimpBone->mNumWeights))
            {
                const uint32_t vertexId = assimpWeight.mVertexId;
                const float weight = assimpWeight.mWeight;
                
                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
    }

    static void LoadMeshes(Gep::Model& model, const aiScene* scene)
    {
        model.meshes.reserve(scene->mNumMeshes);

        for (const aiMesh* assimpMesh : std::span(scene->mMeshes, scene->mNumMeshes))
        {
            Mesh& mesh = model.meshes.emplace_back();
            mesh.name = assimpMesh->mName.C_Str();

            LoadVertices(mesh, assimpMesh);
            LoadIndices(mesh, assimpMesh);
            mesh.CalculateBoundingBox(); //must be done after vertices are loaded

            mesh.materialIndex = gAssimpMaterialIndexToMaterialIndex.at(assimpMesh->mMaterialIndex);
            ExtractBoneWeightForVertices(mesh.vertices, assimpMesh, scene);
        }
    }

    // maps the name of every bone to its inverse bind transformation
    // also used for checking existance of a bone
    static void LoadBoneData(const aiScene* scene)
    {
        for (const aiMesh* mesh : std::span(scene->mMeshes, scene->mNumMeshes))
        {
            for (const aiBone* bone : std::span(mesh->mBones, mesh->mNumBones))
            {
                const std::string name = bone->mName.C_Str();
                gBoneData[name].offset = ToVQS(bone->mOffsetMatrix);
            }
        }
    }

    Model OpenGLRenderer::LoadModelFromFile(const std::filesystem::path& path)
    {
        gBoneData.clear();
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

        // loads all of the materials out of this scene
        LoadMaterials(path, scene);

        // loads every bone name to its offset matrix in gBoneData
        LoadBoneData(scene);

        // fills in the skeleton of the model and the index field in gBoneData
        LoadHierarchy(model, scene);

        LoadMeshes(model, scene);

        LoadAnimations(path.string(), model, scene);

        return model;
    }
}
