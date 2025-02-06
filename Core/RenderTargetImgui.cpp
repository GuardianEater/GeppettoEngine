/*****************************************************************//**
 * \file   RenderTargetImgui.cpp
 * \brief  renders to an imgui window
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"
#include "RenderTargetImgui.hpp"

namespace Gep
{
    void RenderTargetImgui::Draw()
    {
        ImGui::Begin("Render Target");
        ImGui::Image((void*)(intptr_t)GetTexture(), ImVec2(mWidth, mHeight));
        ImGui::End();
    }
}
