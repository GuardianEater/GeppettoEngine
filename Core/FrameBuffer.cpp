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

    FrameBuffer FrameBuffer::CreateSimple(const glm::ivec2 size)
    {
        FrameBuffer fb = Create(size);
        fb.AddTexture(GL_COLOR_ATTACHMENT0, GL_RGBA32F, GL_RGBA, GL_FLOAT);
        fb.AddTexture(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
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

        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexImage2D(GL_TEXTURE_2D, 0, texture.internalFormat, mSize.x, mSize.y, 0, texture.format, texture.type, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, mTarget->frameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, texture.attachment, GL_TEXTURE_2D, texture.id, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Log::Error("AddTexture() error: Framebuffer is not complete!");
        }

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

    // takes the texture attachments and binds them to the corresponding texture units
    void FrameBuffer::BindTextures() const
    {
        for (size_t i = 0; i < mTarget->textures.size(); ++i)
        {
            glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
            glBindTexture(GL_TEXTURE_2D, mTarget->textures[i].id);
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

    void FrameBuffer::Unbind() const
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
        mSize = size;

        glBindFramebuffer(GL_FRAMEBUFFER, mTarget->frameBuffer);

        for (const TextureAttachment& tex : mTarget->textures)
        {
            glBindTexture(GL_TEXTURE_2D, tex.id);
            glTexImage2D(GL_TEXTURE_2D, 0, tex.internalFormat, size.x, size.y, 0, tex.format, tex.type, nullptr);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Log::Error("Resize() error: Framebuffer is not complete!");
        }

        UpdateViewport();
    }

    void FrameBuffer::UpdateViewport() const
    {
        glViewport(0, 0, mSize.x, mSize.y);
    }
}

