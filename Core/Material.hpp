/*****************************************************************//**
 * \file   Material.hpp
 * \brief  Component for storing material data such as color or texture
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <glm.hpp>
#include "Renderer.hpp"

namespace Client
{
    struct Material
    {
        std::string meshName{ "Cube" };
        glm::vec3 diff_coeff = { 2.0f, 2.0f, 2.0f }; // color
        glm::vec3 spec_coeff = { 0.5f, 0.5f, 0.5f }; // shine color
        float spec_exponent = 5; // amount of shine
        bool selected = false;


        void OnImGuiRender(Gep::EngineManager& em)
        {
            const Gep::OpenGLRenderer& renderer = em.GetResource<Gep::OpenGLRenderer>();
            std::vector<std::string> loadedMeshes = renderer.GetLoadedMeshes();

            // drop down for selecting a mesh
            if (ImGui::BeginCombo("Mesh", meshName.c_str()))
            {
                for (const std::string& mesh : loadedMeshes)
                {
                    bool isSelected = mesh == meshName;
                    if (ImGui::Selectable(mesh.c_str(), isSelected))
                    {
                        meshName = mesh;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::ColorEdit3("Diffuse Color", &diff_coeff[0]);
            ImGui::ColorEdit3("Specular Color", &spec_coeff[0]);
            ImGui::SliderFloat("Specular Exponent", &spec_exponent, 0.001f, 10.0f);
        }
    };
}
