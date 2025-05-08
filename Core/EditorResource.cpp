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
    void EditorResource::SmartSelectEntity(Gep::Entity entity)
    {
        // if holding control continue selecting entities
        if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_CONTROL))
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

    bool EditorResource::IsEntitySelected(Gep::Entity entity) const
    {
        return mSelectedEntities.contains(entity);
    }
}
