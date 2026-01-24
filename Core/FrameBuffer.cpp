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

        // create a render buffer and attach as a depth buffer
        glGenRenderbuffers(1, &target.depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, target.depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.x, size.y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.depthBuffer);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return result;
    }

    FrameBuffer FrameBuffer::CreateWithTexture(const glm::ivec2 size)
    {
        FrameBuffer fb = Create(size);
        fb.AddTexture(GL_RGBA32F, GL_FLOAT);
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

    void FrameBuffer::AddTexture(GLint format, GLenum type)
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
        texture.format = format;
        texture.type = type;

        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, mSize.x, mSize.y, 0, GL_RGBA, type, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, mTarget->frameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(mTarget->textures.size() - 1), GL_TEXTURE_2D, texture.id, 0);

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
        for (size_t i = 0; i < mTarget->textures.size(); ++i)
            buffers.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));

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
            glTexImage2D(GL_TEXTURE_2D, 0, tex.format, size.x, size.y, 0, GL_RGBA, tex.type, nullptr);
        }

        // update depth buffer size
        glBindRenderbuffer(GL_RENDERBUFFER, mTarget->depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.x, size.y);

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

