/*****************************************************************//**
 * \file   RenderTargetImgui.hpp
 * \brief  renders to an imgui window
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include "IRenderTarget.hpp"
#include "imgui.h"
#include "RenderTargetTexture.hpp"

namespace Gep
{
    class RenderTargetImgui : public RenderTargetTexture
    {
    public:
        RenderTargetImgui(int width, int height)
            : RenderTargetTexture(width, height)
        {}
        void Draw() override;
    };
}
