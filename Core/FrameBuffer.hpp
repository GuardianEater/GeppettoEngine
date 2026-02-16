/*****************************************************************//**
 * \file   FrameBuffer.hpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>

namespace Gep
{
    struct TextureAttachment
    {
        GLuint id = 0;
        GLuint64 handle = 0; // 64 bit gpu pointer, used to sample the texture on the gpu
        GLenum target = GL_TEXTURE_2D;
        GLint internalFormat = GL_RGBA32F;
        GLint format = GL_RGBA;
        GLenum type = GL_FLOAT;
        GLenum attachment = GL_COLOR_ATTACHMENT0;
    };

    // wrapper around an opengl framebuffer
    class FrameBuffer
    {
    public:
        static FrameBuffer CreateDepthCubeMap(const glm::ivec2 size);
        static FrameBuffer CreateWithTexture(const glm::ivec2 size, GLenum attachment, GLint internalFormat, GLint format, GLenum type);
        static FrameBuffer Create(const glm::ivec2 size);
        static FrameBuffer CreateSimple(const glm::ivec2 size);
        static const FrameBuffer& Default(); // returns the default frame buffer (the screen) its ok to copy this it will always reference the same underlying data

        void AddTexture(GLenum attachment, GLint internalFormat, GLint format, GLenum type); // adds a texture attachment to the framebuffer
        GLuint GetTexture(size_t index) const; // gets the opengl texture id of the texture attachment at the given index
        GLint GetTextureInternalFormat(size_t index) const;
        GLint GetTextureFormat(size_t index) const;
        GLenum GetTextureType(size_t index) const;
        size_t GetTextureCount() const { return mTarget ? mTarget->textures.size() : 0; }
        const std::vector<TextureAttachment>& GetTextureAttachments() const { return mTarget->textures; }

        GLuint GetFrameBufferID() const { return mTarget ? mTarget->frameBuffer : 0; }

        void BindTextures() const; // binds the texture attachments to the corresponding texture units
        void DrawBuffers() const; // sets the opengl draw buffers to the texture attachments

        void Bind() const;
        static void Unbind();
        void Clear(const glm::vec4& color = { 0.0f, 0.0f, 0.0f, 1.0f }) const;

        // changes the size of the framebuffer and its attachments, does nothing if the size hasn't changed
        void Resize(glm::ivec2 newSize);
        void UpdateViewport() const;

        glm::ivec2 GetSize() const { return mSize; }

    private:

        struct TargetData
        {
            GLuint frameBuffer = 0;
            GLuint depthBuffer = 0;

            std::vector<TextureAttachment> textures;

            ~TargetData()
            {
                glDeleteFramebuffers(1, &frameBuffer);
                glDeleteRenderbuffers(1, &depthBuffer);

                for (const TextureAttachment& tex : textures)
                    glDeleteTextures(1, &tex.id);
            }
        };

    private:
        // this is so if the frame buffer is copied it will still reference the same underlying data, also avoids delete issues
        std::shared_ptr<TargetData> mTarget;

        glm::ivec2 mSize = { 0, 0 };
    };
}
