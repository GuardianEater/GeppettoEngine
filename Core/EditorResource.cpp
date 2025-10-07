/*****************************************************************//**
 * \file   EditorResource.cpp
 * \brief  impementation of the editor resource
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#include "pch.hpp"
#include "EditorResource.hpp"

namespace Client
{
    void EditorResource::SmartSelectEntity(Gep::Entity entity, GLFWwindow* window)
    {
        if (!window)
            window = glfwGetCurrentContext();

        // if holding control continue selecting entities
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
        {
            // if already selected deselect
            if (IsEntitySelected(entity))
                DeselectEntity(entity);
            else
                SelectAnotherEntity(entity);
        }
        // regular select
        else
        {
            SelectEntity(entity);
        }
    }
    void EditorResource::SelectEntity(Gep::Entity entity)
    {
        mSelectedEntities.clear();
        mSelectedEntities.insert(entity);
    }

    void EditorResource::DeselectEntity(Gep::Entity entity)
    {
        if (mSelectedEntities.contains(entity))
            mSelectedEntities.erase(entity);
    }

    void EditorResource::SelectAll(std::span<const Gep::Entity> entities)
    {
        mSelectedEntities.clear();
        mSelectedEntities.insert(entities.begin(), entities.end());
    }

    void EditorResource::DeselectAll()
    {
        mSelectedEntities.clear();
    }

    void EditorResource::SelectAnotherEntity(Gep::Entity entity)
    {
        mSelectedEntities.insert(entity);
    }

    const std::unordered_set<Gep::Entity>& EditorResource::GetSelectedEntities() const
    {
        return mSelectedEntities;
    }

    void EditorResource::LabledInput_Float(const std::string& label, float* v, float columnWidth, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
    {
        LabledInput_Setup(label, columnWidth);
        ImGui::DragFloat(std::string("##").append(label).append(std::to_string(reinterpret_cast<uint64_t>(v))).c_str(), v, v_speed, v_min, v_max, format, flags);
        LabledInput_End();
    }

    void EditorResource::LabledInput_Float3(const std::string& label, float* v, float columnWidth, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
    {
        LabledInput_Setup(label, columnWidth);
        ImGui::DragFloat3(std::string("##").append(label).append(std::to_string(reinterpret_cast<uint64_t>(v))).c_str(), v, v_speed, v_min, v_max, format, flags);
        LabledInput_End();
    }

    void EditorResource::LabledInput_Float4(const std::string& label, float* v, float columnWidth, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
    {
        LabledInput_Setup(label, columnWidth);
        ImGui::DragFloat4(std::string("##").append(label).append(std::to_string(reinterpret_cast<uint64_t>(v))).c_str(), v, v_speed, v_min, v_max, format, flags);
        LabledInput_End();
    }

    void EditorResource::LabledInput_Setup(const std::string& label, float columnWidth)
    {
        ImGui::Columns(2, "input_columns", false);
        ImGui::SetColumnWidth(0, columnWidth * ImGui::GetIO().FontGlobalScale);

        // Calculate height difference to center text
        float lineHeight = ImGui::GetTextLineHeight();
        float frameHeight = ImGui::GetFrameHeight();
        float verticalOffset = (frameHeight - lineHeight) * 0.5f;

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + verticalOffset);
        ImGui::TextUnformatted(label.c_str());
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1); // fill remaining space in the column

    }

    void EditorResource::LabledInput_End()
    {
        ImGui::Columns(1);
    }

    bool EditorResource::IsEntitySelected(Gep::Entity entity) const
    {
        return mSelectedEntities.contains(entity);
    }
}
