/*****************************************************************//**
 * \file   FrameBuffer.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#include "pch.hpp"
#include "RenderTarget.hpp"

namespace Gep
{
    FrameBuffer FrameBuffer::Create(const glm::ivec2 size)
    {
        FrameBuffer result;
        result.mTarget = std::make_shared<TargetData>();
        TargetData& target = *result.mTarget;

        // generate framebuffer
        glGenFramebuffers(1, &target.frameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, target.frameBuffer);

        // create a render buffer and attach as a depth buffer
        glGenRenderbuffers(1, &target.depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, target.depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.x, size.y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.depthBuffer);

        // create the texture
        glGenTextures(1, &target.texture);
        glBindTexture(GL_TEXTURE_2D, target.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Log::Error("Create() error: Framebuffer is not complete!");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return result;
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
        if (mSize == size) return; // dont do anything if the size hasn't changed
        mSize = size;

        glBindTexture(GL_TEXTURE_2D,  mTarget->texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
        glBindRenderbuffer(GL_RENDERBUFFER, mTarget->depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.x, size.y);

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

