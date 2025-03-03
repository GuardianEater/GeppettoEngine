/*****************************************************************//**
 * \file   TextureComponent.hpp
 * \brief  adds a texture to an entity
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include <string>

namespace Client
{
    struct Texture
    {
        std::filesystem::path texturePath = "assets\\textures\\Checker.jpg";

        void OnImGuiRender(Gep::EngineManager& em)
        {
            const Gep::OpenGLRenderer& renderer = em.GetResource<Gep::OpenGLRenderer>();
            std::vector<std::filesystem::path> loadedTextures = renderer.GetLoadedTextures();

            bool texturesOpen = ImGui::BeginCombo("Textures", texturePath.string().c_str());

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(char) * (strlen((const char*)payload->Data) + 1));
                    const char* path = (const char*)payload->Data;
                    std::filesystem::path droppedPath(path);

                    texturePath = droppedPath;
                }
                ImGui::EndDragDropTarget();
            }

            if (texturesOpen)
            {
                for (const auto& loadedTexturePath : loadedTextures)
                {
                    bool isSelected = loadedTexturePath == texturePath;
                    if (ImGui::Selectable(loadedTexturePath.string().c_str(), isSelected))
                    {
                        texturePath = loadedTexturePath;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
    };
}
