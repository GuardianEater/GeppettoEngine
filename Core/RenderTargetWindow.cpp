/*****************************************************************//**
 * \file   RenderTargetWindow.cpp
 * \brief  renders to the primary window
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"

#include "RenderTargetWindow.hpp"

namespace Gep
{
    void RenderTargetWindow::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        int width, height;
        glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
        glViewport(0, 0, width, height);
    }
    void RenderTargetWindow::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderTargetWindow::Clear(const glm::vec3& color)
    {
        glClearColor(color.r, color.g, color.b, 1);
        glClearDepth(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RenderTargetWindow::Draw(EngineManager& em, Entity camera)
    {
        // check if the window has changed size
        int width, height;
        glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
        if (width != mSize.x || height != mSize.y)
        {
            mSize.x = width;
            mSize.y = height;
            glViewport(0, 0, width, height);
        }
    }
}
