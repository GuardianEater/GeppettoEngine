/*****************************************************************//**
 * \file   ImGuiHelp2.hpp
 * \brief  utilities for drawing various engine structs
 * 
 * \author 2018t
 * \date   April 2026
 *********************************************************************/

#include "Material.hpp"
#include <imgui.h>

namespace Gep::Gui
{
    inline void DrawMaterial(Gep::Material& mat)
    {
        const ImVec2 imageSize = { 64 * ImGui::GetStyle().FontScaleMain, 64 * ImGui::GetStyle().FontScaleMain };

        if (mat.aoTexture.id)
        {
            ImGui::Image(mat.aoTexture.id, imageSize);
            ImGui::SameLine();

            std::string aoTextureIdStr = std::to_string(mat.aoTexture.id);
            ImGui::BeginGroup();
            ImGui::Text("Ambient Occlusion");
            ImGui::Text(aoTextureIdStr.c_str());
            ImGui::EndGroup();
        }
        else
        {
            ImVec4 color{ mat.ao, mat.ao, mat.ao, 1.0f };
            ImGui::ColorButton("ao", color, 0, imageSize);
            ImGui::SameLine();

            std::string aoStr = std::to_string(mat.ao);
            ImGui::BeginGroup();
            ImGui::Text("Ambient Occlusion");
            ImGui::Text(aoStr.c_str());
            ImGui::EndGroup();
        }

        if (mat.diffuseTexture.id)
        {
            ImGui::Image(mat.diffuseTexture.id, imageSize);
            ImGui::SameLine();

            std::string diffuseTextureIdStr = std::to_string(mat.diffuseTexture.id);
            ImGui::BeginGroup();
            ImGui::Text("Diffuse");
            ImGui::Text(diffuseTextureIdStr.c_str());
            ImGui::EndGroup();
        }
        else
        {
            ImVec4 color{ mat.color.r, mat.color.g, mat.color.b, mat.color.a };
            ImGui::ColorButton("diffuse", color, 0, imageSize);
            ImGui::SameLine();

            std::string diffuseStr =
                "(" + std::to_string(mat.color.r) +
                "," + std::to_string(mat.color.g) +
                "," + std::to_string(mat.color.b) +
                "," + std::to_string(mat.color.a) + ")";

            ImGui::BeginGroup();
            ImGui::Text("Diffuse");
            ImGui::Text(diffuseStr.c_str());
            ImGui::EndGroup();
        }

        if (mat.metalnessTexture.id)
        {
            ImGui::Image(mat.metalnessTexture.id, imageSize);
            ImGui::SameLine();

            std::string metalnessTextureIdStr = std::to_string(mat.metalnessTexture.id);
            ImGui::BeginGroup();
            ImGui::Text("Metalness");
            ImGui::Text(metalnessTextureIdStr.c_str());
            ImGui::EndGroup();
        }
        else
        {
            ImVec4 color{ mat.metalness, mat.metalness, mat.metalness, 1.0f };
            ImGui::ColorButton("metalness", color, 0, imageSize);
            ImGui::SameLine();

            std::string metalnessStr = std::to_string(mat.metalness);
            ImGui::BeginGroup();
            ImGui::Text("Metalness");
            ImGui::Text(metalnessStr.c_str());
            ImGui::EndGroup();
        }

        if (mat.normalTexture.id)
        {
            ImGui::Image(mat.normalTexture.id, imageSize);
            ImGui::SameLine();

            std::string normalTextureIdStr = std::to_string(mat.normalTexture.id);
            ImGui::BeginGroup();
            ImGui::Text("Normals");
            ImGui::Text(normalTextureIdStr.c_str());
            ImGui::EndGroup();
        }
        else
        {
            // display nothing if no normal texture
        }

        if (mat.roughnessTexture.id)
        {
            ImGui::Image(mat.roughnessTexture.id, imageSize);
            ImGui::SameLine();

            std::string roughnessTextureIdStr = std::to_string(mat.roughnessTexture.id);
            ImGui::BeginGroup();
            ImGui::Text("Roughness");
            ImGui::Text(roughnessTextureIdStr.c_str());
            ImGui::EndGroup();
        }
        else
        {
            ImVec4 color{ mat.roughness, mat.roughness, mat.roughness, 1.0f };
            ImGui::ColorButton("roughness", color, 0, imageSize);
            ImGui::SameLine();

            std::string roughnessStr = std::to_string(mat.roughness);
            ImGui::BeginGroup();
            ImGui::Text("Roughness");
            ImGui::Text(roughnessStr.c_str());
            ImGui::EndGroup();
        }

        if (mat.emissionTexture.id)
        {
            ImGui::Image(mat.emissionTexture.id, imageSize);
            ImGui::SameLine();

            std::string emissionTextureIdStr = std::to_string(mat.emissionTexture.id);
            ImGui::BeginGroup();
            ImGui::Text("Emission");
            ImGui::Text(emissionTextureIdStr.c_str());
            ImGui::EndGroup();
        }
        else
        {
            ImVec4 color{ mat.emission, mat.emission, mat.emission, 1.0f };
            ImGui::ColorButton("emission", color, 0, imageSize);
            ImGui::SameLine();

            std::string emissionStr = std::to_string(mat.emission);
            ImGui::BeginGroup();
            ImGui::Text("Emission");
            ImGui::Text(emissionStr.c_str());
            ImGui::EndGroup();
        }
    }
}
