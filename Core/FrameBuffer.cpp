/*****************************************************************//**
 * \file   FrameBuffer.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#include "pch.hpp"
#include "FrameBuffer.hpp"

namespace Gep
{
    [[nodiscard]] FrameBuffer FrameBuffer::CreateDepthCubeMap(const glm::ivec2 size)
    {
        FrameBuffer result = Create(size);
        TextureAttachment& texture = result.mTarget->textures.emplace_back();
        texture.internalFormat = GL_DEPTH_COMPONENT;
        texture.format = GL_DEPTH_COMPONENT;
        texture.type = GL_FLOAT;
        texture.attachment = GL_DEPTH_ATTACHMENT;
        texture.target = GL_TEXTURE_CUBE_MAP;

        // generate cube map
        glGenTextures(1, &texture.id);
        glBindTexture(texture.target, texture.id);

        // generate the 6 faces of the cube map, with depth component format
        for (uint32_t i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, texture.internalFormat, size.x, size.y, 0, texture.format, texture.type, nullptr);

        glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(texture.target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(texture.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(texture.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // set handle for sampling the texture on the gpu
        texture.handle = glGetTextureHandleARB(texture.id);
        glMakeTextureHandleResidentARB(texture.handle);

        glBindFramebuffer(GL_FRAMEBUFFER, result.mTarget->frameBuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, texture.attachment, texture.id, 0);

        // these are here to tell opengl that we aren't writing to a color buffer
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return result;
    }

    [[nodiscard]] FrameBuffer FrameBuffer::Create(const glm::ivec2 size)
    {
        FrameBuffer result;
        result.mTarget = std::make_shared<TargetData>();
        result.mSize = size;
        TargetData& target = *result.mTarget;

        // generate framebuffer
        glGenFramebuffers(1, &target.frameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, target.frameBuffer);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return result;
    }

    [[nodiscard]] FrameBuffer FrameBuffer::CreateSimple(const glm::ivec2 size)
    {
        FrameBuffer fb = Create(size);
        fb.AddTexture(GL_COLOR_ATTACHMENT0, GL_RGBA32F, GL_RGBA, GL_FLOAT);
        fb.AddTexture(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT);
        return fb;
    }

    [[nodiscard]] FrameBuffer FrameBuffer::CreateWithTexture(const glm::ivec2 size, GLenum attachment, GLint internalFormat, GLint format, GLenum type)
    {
        FrameBuffer fb = Create(size);
        fb.AddTexture(attachment, internalFormat, format, type);
        return fb;
    }

    const FrameBuffer& FrameBuffer::Default()
    {
        static FrameBuffer defaultBuffer = []()
        {
            FrameBuffer fb;
            fb.mTarget = std::make_shared<TargetData>();
            fb.mTarget->frameBuffer = 0;
            fb.mTarget->depthBuffer = 0;
            return fb;
        }();
        
        if (GLFWwindow* window = glfwGetCurrentContext())
        {
            int width = 0;
            int height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            defaultBuffer.mSize = { width, height };
        }
        else
        {
            defaultBuffer.mSize = { 0, 0 };
        }

        return defaultBuffer;
    }

    void FrameBuffer::AddTexture(GLenum attachment, GLint internalFormat, GLint format, GLenum type)
    {
        if (!mTarget)
        {
            Log::Error("AddTexture() error: FrameBuffer has not been created!");
            return;
        }

        if (mTarget->frameBuffer == 0)
        {
            Log::Error("AddTexture() error: Cannot add texture to the default frame buffer!");
            return;
        }

        TextureAttachment& texture = mTarget->textures.emplace_back();
        texture.internalFormat = internalFormat;
        texture.format = format;
        texture.type = type;
        texture.attachment = attachment;
        texture.target = GL_TEXTURE_2D;

        // generate texture
        glGenTextures(1, &texture.id);
        glBindTexture(texture.target, texture.id);

        glTexImage2D(texture.target, 0, texture.internalFormat, mSize.x, mSize.y, 0, texture.format, texture.type, nullptr);

        // set texture parameters
        glTexParameteri(texture.target, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(texture.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(texture.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, mTarget->frameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, texture.attachment, texture.target, texture.id, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Log::Error("AddTexture() error: Framebuffer is not complete!");
        }

        glBindTexture(texture.target, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint FrameBuffer::GetTexture(size_t index) const
    {
        if (index >= mTarget->textures.size())
        {
            Log::Error("GetTexture() error: Texture index out of range!");
            return 0;
        }
        return mTarget->textures[index].id;
    }

    GLint FrameBuffer::GetTextureInternalFormat(size_t index) const
    {
        if (index >= mTarget->textures.size())
        {
            Log::Error("GetTexture() error: Texture index out of range!");
            return 0;
        }
        return mTarget->textures[index].internalFormat;
    }

    GLint FrameBuffer::GetTextureFormat(size_t index) const
    {
        if (index >= mTarget->textures.size())
        {
            Log::Error("GetTexture() error: Texture index out of range!");
            return 0;
        }
        return mTarget->textures[index].format;
    }

    GLenum FrameBuffer::GetTextureType(size_t index) const
    {
        if (index >= mTarget->textures.size())
        {
            Log::Error("GetTexture() error: Texture index out of range!");
            return 0;
        }
        return mTarget->textures[index].type;
    }

    const std::vector<TextureAttachment>& FrameBuffer::GetTextureAttachments() const
    {
        return mTarget->textures;
    }

    // takes the texture attachments and binds them to the corresponding texture units
    void FrameBuffer::BindTextures() const
    {
        for (size_t i = 0; i < mTarget->textures.size(); ++i)
        {
            glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
            glBindTexture(mTarget->textures[i].target, mTarget->textures[i].id);
        }
    }

    void FrameBuffer::DrawBuffers() const
    {
        std::vector<GLenum> buffers;
        buffers.reserve(mTarget->textures.size());

        for (const TextureAttachment& tex : mTarget->textures)
        {
            if (tex.attachment >= GL_COLOR_ATTACHMENT0 && tex.attachment <= GL_COLOR_ATTACHMENT15)
                buffers.push_back(tex.attachment);
        }

        if (!buffers.empty())
            glDrawBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
    }

    void FrameBuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mTarget->frameBuffer);
    }

    void FrameBuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::Clear(const glm::vec4& color) const
    {
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void FrameBuffer::Resize(glm::ivec2 size)
    {
        if (!mTarget)
        {
            Log::Error("Resize() error: FrameBuffer has not been created!");
            return;
        }

        if (mTarget->frameBuffer == 0)
        {
            Log::Error("Resize() error: Cannot resize the default frame buffer!");
            return;
        }

        if (mSize == size) return; // dont do anything if the size hasn't changed
        Gep::Log::Info("Resizing Framebuffer...[", size.x, ", ", size.y, "]");
        mSize = size;

        glBindFramebuffer(GL_FRAMEBUFFER, mTarget->frameBuffer);

        for (const TextureAttachment& tex : mTarget->textures)
        {
            glBindTexture(tex.target, tex.id);
            glTexImage2D(tex.target, 0, tex.internalFormat, size.x, size.y, 0, tex.format, tex.type, nullptr);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Log::Error("Resize() error: Framebuffer is not complete!");
        }
    }


    void FrameBuffer::UpdateViewport() const
    {
        glViewport(0, 0, mSize.x, mSize.y);
    }
}

