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
        std::string textureName = "Checker";

        void OnImGuiRender(Gep::EngineManager& em)
        {
            const Gep::OpenGLRenderer& renderer = em.GetResource<Gep::OpenGLRenderer>();
            std::vector<std::string> loadedTextures = renderer.GetLoadedTextures();

            if (ImGui::BeginCombo("Textures", textureName.c_str()))
            {
                for (const std::string& texture : loadedTextures)
                {
                    bool isSelected = texture == textureName;
                    if (ImGui::Selectable(texture.c_str(), isSelected))
                    {
                        textureName = texture;
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
