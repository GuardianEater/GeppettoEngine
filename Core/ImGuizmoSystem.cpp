/*****************************************************************//**
 * \file   ImGuizmoSystem.cpp
 * \brief  gizmo system for the engine
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"
#include "ImGuizmoSystem.hpp"

#include <ImGuizmo.h>

namespace Client
{
    void ImGuizmoSystem::Initialize()
    {
    }

    void ImGuizmoSystem::Exit()
    {
    }

    void ImGuizmoSystem::Update(float dt)
    {
        ImGuizmo::BeginFrame();
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;

        ImGuizmo::SetRect(0, 0, displaySize.x, displaySize.y);

    }
}
