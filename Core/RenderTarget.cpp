/*****************************************************************//**
 * \file   RenderTarget.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#include "pch.hpp"
#include "RenderTarget.hpp"

namespace Gep
{
    RenderTarget RenderTarget::Generate(glm::ivec2 size)
    {
        RenderTarget result;
        result.mTarget = std::make_shared<RenderTarget::TargetData>();
        RenderTarget::TargetData& target = *result.mTarget;

        glGenFramebuffers(1, &target.mFrameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, target.mFrameBuffer);

        // create the texture
        glGenTextures(1, &target. mTexture);
        glBindTexture(GL_TEXTURE_2D, target.mTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, result.mSize.x, result.mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.mTexture, 0);

        // create the render buffer
        glGenRenderbuffers(1, &target.mRenderBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, target.mRenderBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, result.mSize.x, result.mSize.y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.mRenderBuffer);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Log::Error("Framebuffer is not complete!");
        }

        return result;
    }

    void RenderTarget::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mTarget->mFrameBuffer);
        glViewport(0, 0, mSize.x, mSize.y);
    }

    void RenderTarget::Unbind() const
    {
        // no-op
    }

    void RenderTarget::Clear(const glm::vec3& color) const
    {
        glClearColor(color.r, color.g, color.b, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }


    void RenderTarget::Resize(glm::ivec2 newSize)
    {
        if (mSize == newSize) return; // dont do anything if the size hasn't changed
        mSize = newSize;

        glBindTexture(GL_TEXTURE_2D,  mTarget->mTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, newSize.x, newSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindRenderbuffer(GL_RENDERBUFFER, mTarget->mRenderBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, newSize.x, newSize.y);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Log::Error("Framebuffer is not complete!");
        }
    }
}

