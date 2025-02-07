/*****************************************************************//**
 * \file   RenderTargetWindow.hpp
 * \brief  render target that renders to the primary window
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include "IRenderTarget.hpp"

namespace Gep
{
    class RenderTargetWindow : public IRenderTarget
    {
    public:
        RenderTargetWindow(int width, int height)
            : IRenderTarget(width, height)
        {}

        void Bind() override;
        void Unbind() override;
        void Clear(const glm::vec3& color) override;
        void Draw() override;
    };
}
