/*****************************************************************//**
 * \file   ImGuiSystem.inl
 * \brief  
 * 
 * \author Travis Gronvold
 * \date   January 2025
 *********************************************************************/

#pragma once

#include "ImGuiSystem.hpp"
#include "Events.hpp"
namespace Client
{
    template <typename... ComponentTypes>
    void ImGuiSystem::OnComponentsRegistered(Gep::TypeList<ComponentTypes...> componentTypes)
    {
        componentTypes.for_each([&]<typename ComponentType>()
        {
            mComponentInspectorPanels.push_back([&](std::span<Gep::Entity> entities)
            {
                std::string componentName = Gep::GetTypeInfo<ComponentType>().PrettyName();

                bool open = ImGui::CollapsingHeader(componentName.c_str());

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Delete"))
                    {
                        for (auto entity : entities)
                            mManager.DestroyComponent<ComponentType>(entity);
                    }
                    ImGui::EndPopup();
                    return;
                }

                if (open)
                {
                    // the entitiy must have the component, because the component count and entity count are must be the same
                    std::vector<ComponentType*> components;
                    for (auto entity : entities)
                        components.push_back(&mManager.GetComponent<ComponentType>(entity));

                    mManager.SignalEvent(Gep::Event::ComponentEditorRender<ComponentType>({ entities, components }));

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                }
            });
        });
    }
}
