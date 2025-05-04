/*****************************************************************//**
 * \file   RenderTargetImgui.hpp
 * \brief  renders to an imgui window
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
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

        void Draw(EngineManager& em, Entity camera, const std::function<void()>& drawFunction = []() {}) override;
    };
}
