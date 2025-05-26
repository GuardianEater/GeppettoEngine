/*****************************************************************//**
 * \file   Renderer.cpp
 * \brief  Base interface for the type of rendering being performed
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "pch.hpp"

#include "Renderer.hpp"

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "shellapi.h"
#undef LoadImage
#undef min
#undef max

#include "ObjMesh.hpp"
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

    void OpenGLRenderer::LoadMesh(const std::string& name, const Mesh& mesh)
    {
        if (mMeshNameToID.contains(name))
        {
            Gep::Log::Error("Cannot load mesh: [", name, "] a mesh with that name has already been loaded");
            return;
        }

        uint64_t id = mMeshDatas.emplace();
        mMeshNameToID[name] = id;
        MeshData& meshData = mMeshDatas.at(id);

        meshData.GenVertexBuffer(mesh);
        meshData.GenFaceBuffer(mesh);
        meshData.BindBuffers();
    }

    void OpenGLRenderer::LoadMesh(const std::filesystem::path& path)
    {
        std::string strPath = path.string();
        if (mMeshNameToID.contains(strPath))
        {
            Gep::Log::Error("Cannot load mesh: [", path.string(), "] a mesh with that name has already been loaded");
            return;
        }

        std::string ext = path.extension().string();

        Mesh mesh = Gep::LoadMesh(path);
        LoadMesh(path.string(), mesh);
    }

    uint64_t OpenGLRenderer::GetMesh(const std::string& name) const
    {
        if (!mMeshNameToID.contains(name))
        {
            Gep::Log::Error("Cannot get mesh: [", name, "] a mesh with that name has not been loaded");
            return 0;
        }

        return mMeshNameToID.at(name);
    }

    uint64_t OpenGLRenderer::GetOrLoadMesh(const std::string& meshName)
    {
        if (!mMeshNameToID.contains(meshName))
            LoadMesh(std::filesystem::path(meshName));

        return GetMesh(meshName);
    }

    bool OpenGLRenderer::IsMeshLoaded(const std::string& name) const
    {
        return mMeshNameToID.contains(name);
    }

    void OpenGLRenderer::SetShader(const std::string& name)
    {
        mActiveShaderName = ""; // incase this function fails make sure the shader is set to nothing first

        std::string fragPath = "assets\\shaders\\" + name + ".frag";
        std::string vertPath = "assets\\shaders\\" + name + ".vert";

        // load a new shader if
        if (!mShaders.contains(name))
        {
            auto [it, newInsertion] = mShaders.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(vertPath, fragPath));
            if (!it->second.IsValid())
            {
                Gep::Log::Error("Shader at path: [", fragPath, "] and [", vertPath, "] did not compile");
                return;
            }
        }

        mActiveShaderName = name;
    }

    void OpenGLRenderer::ToggleWireframes()
    {
        mWireframeMode = !mWireframeMode;
    }

    void OpenGLRenderer::UnloadMesh(const std::string& name)
    {
        if (!mMeshNameToID.contains(name))
        {
            Gep::Log::Error("Cannot unload mesh: [", name, "] a mesh with that name has not been loaded");
            return;
        }

        uint64_t id = mMeshNameToID.at(name);
        MeshData& meshData = mMeshDatas.at(id);
        meshData.DeleteBuffers();
        mMeshDatas.erase(id);
        mMeshNameToID.erase(name);
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



        glUniformMatrix4fv(0, 1, false, &pers[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniform4fv(4, 1, &eye[0]);
        glUseProgram(0);
    }

    void OpenGLRenderer::SetTexture(GLuint texture)
    {
        Shader& activeShader = mShaders.at(mActiveShaderName);

        activeShader.Use([&]()
        {
            glBindTexture(GL_TEXTURE_2D, texture);
        });

        activeShader.SetUniform("isUsingTexture", mTexturesEnabled);
    }

    void OpenGLRenderer::SetHighlight(bool highlight)
    {
        mNextMeshIsHighlighted = highlight;
    }

    void OpenGLRenderer::SetSolidColor(const glm::vec3& color)
    {
        Shader& activeShader = mShaders.at(mActiveShaderName);

        activeShader.SetUniform("isSolidColor", true);
        activeShader.SetUniform("solidColor", color);
    }

    void OpenGLRenderer::SetIgnoreLight(bool ignore)
    {
        Shader& activeShader = mShaders.at(mActiveShaderName);

        activeShader.SetUniform("isIgnoringLight", ignore);
    }

    void OpenGLRenderer::SetCamera(const glm::mat4& pers, const glm::mat4& view, const glm::vec3& eye)
    {
        mNextPerspective = pers;
        mNextView = view;
        mNextEye = glm::vec4(eye, 1.0f);
    }

    void OpenGLRenderer::SetModel(const glm::mat4& modelingMatrix)
    {
        Shader& activeShader = mShaders.at(mActiveShaderName);

        glm::mat4 normal = glm::mat4(glm::mat3(affine_inverse(modelingMatrix)));

        activeShader.SetUniform("modelMatrix", modelingMatrix);
        activeShader.SetUniform("normalMatrix", normal, true);
    }

    void OpenGLRenderer::SetWireframe(bool wireframe)
    {
        mNextMeshIsWireframe = wireframe;
    }

    void OpenGLRenderer::SetBackfaceCull(bool backfaceCull)
    {
        mNextMeshIsBackfaceCulling = backfaceCull;
    }

    void OpenGLRenderer::SetMaterial(const PBRMaterial& material)
    {
        Shader& activeShader = mShaders.at(mActiveShaderName);
        
        activeShader.SetUniform("material.ao", material.ao);
        activeShader.SetUniform("material.roughness", material.roughness);
        activeShader.SetUniform("material.metallic", material.metalness);
        activeShader.SetUniform("material.color", material.color);
    }

    // toggle textures
    void OpenGLRenderer::ToggleTextures()
    {
        mTexturesEnabled = !mTexturesEnabled;
    }

    std::vector<std::string> OpenGLRenderer::GetLoadedMeshes() const
    {
        std::vector<std::string> meshes;

        for (const auto& [name, id] : mMeshNameToID)
        {
            meshes.emplace_back(name);
        }

        return meshes;
    }

    std::vector<std::filesystem::path> OpenGLRenderer::GetLoadedTextures() const
    {
        std::vector<std::filesystem::path> textures;

        for (const auto& [name, _] : mTextures)
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
        static std::vector<std::string> allowedExtensions = { ".jpg", ".jpeg", ".png", ".bmp"};

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
        if (!std::filesystem::exists(texturePath)) {
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
        if (!image) {
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
            return 0;
        }

        return mTextures.at(texturePath);
    }

    GLuint OpenGLRenderer::GetOrLoadTexture(const std::filesystem::path& texturePath)
    {
        if (!mTextures.contains(texturePath))
            LoadTexture(texturePath);

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

    void OpenGLRenderer::DrawMesh(uint64_t meshID)
    {
        if (!mMeshDatas.contains(meshID))
        {
            Gep::Log::Error("Cannot draw mesh: [", meshID, "] a mesh with that id has not been loaded");
            return;
        }

        const MeshData& md = mMeshDatas.at(meshID);
        constexpr std::uint64_t faceSize = sizeof(Mesh::Face) / sizeof(GLuint);
        Shader& activeShader = mShaders.at(mActiveShaderName);

        activeShader.SetUniform("lightCount", static_cast<int>(mLightData.size()));
        activeShader.SetUniform("perspectiveMatrix", mNextPerspective);
        activeShader.SetUniform("viewMatrix", mNextView);
        activeShader.SetUniform("camPosition", mNextEye);

        if (mNextMeshIsBackfaceCulling)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }
        else
            glDisable(GL_CULL_FACE);

        activeShader.Use([&]() 
        {
            glBindVertexArray(md.mVertexArrayObject);

            // If outlining is enabled, render the outline first
            if (mNextMeshIsHighlighted)
            {
                activeShader.SetUniform("isHighlighted", true);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawElements(GL_TRIANGLES, faceSize * md.mFaceCount, GL_UNSIGNED_INT, 0);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                activeShader.SetUniform("isHighlighted", false);
            }

            if (mWireframeMode || mNextMeshIsWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, faceSize * md.mFaceCount, GL_UNSIGNED_INT, 0);
            if (mWireframeMode || mNextMeshIsWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            glBindVertexArray(0);
        });

        activeShader.SetUniform("isUsingTexture", false);
        activeShader.SetUniform("isSolidColor", false);

        mNextMeshIsWireframe = false;
        mNextMeshIsBackfaceCulling = true;
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
