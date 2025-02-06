/*****************************************************************//**
 * \file   RenderTargetTexture.cpp
 * \brief  render target that renders to a texture
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"
#include "RenderTargetTexture.hpp"

namespace Gep
{
    RenderTargetTexture::RenderTargetTexture(int width, int height)
        : IRenderTarget(width, height)
    {
        glGenFramebuffers(1, &mFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

        // create the texture
        glGenTextures(1, &mTexture);
        glBindTexture(GL_TEXTURE_2D, mTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture, 0);


        // create the render buffer
        glGenRenderbuffers(1, &mRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, mRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Log::Error("Framebuffer is not complete!");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    RenderTargetTexture::~RenderTargetTexture()
    {
        glDeleteFramebuffers(1, &mFBO);
        glDeleteTextures(1, &mTexture);
        glDeleteRenderbuffers(1, &mRBO);
    }

    void RenderTargetTexture::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    }

    void RenderTargetTexture::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderTargetTexture::Clear()
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RenderTargetTexture::Resize(int width, int height)
    {
        mWidth = width;
        mHeight = height;
        glBindTexture(GL_TEXTURE_2D, mTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glBindRenderbuffer(GL_RENDERBUFFER, mRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Log::Error("Framebuffer is not complete!");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint RenderTargetTexture::GetTexture() const
    {
        return mTexture;
    }

    void RenderTargetTexture::Draw()
    {

    }

}
