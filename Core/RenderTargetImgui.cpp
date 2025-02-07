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
        if (ImGui::Begin("Render Target"))
        {
            ImVec2 size = ImGui::GetContentRegionAvail();

            if (size.x != mSize.x || size.y != mSize.y)
            {
                mSize.x = size.x;
                mSize.y = size.y;
                glViewport(0, 0, size.x, size.y);
            }

            ImGui::Image((void*)(intptr_t)GetTexture(), size, ImVec2(0, 1), ImVec2(1, 0));
        }
        ImGui::End();
    }
}
