/*****************************************************************//**
 * \file   Mesh.hpp
 * \brief  Component for storing material data such as color or texture
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <glm\glm.hpp>
#include <EngineManager.hpp>
#include <imgui.h>
#include "Renderer.hpp"
#include "EditorResource.hpp"

namespace Client
{
    struct Mesh
    {
        std::string meshName{ "Cube" };
        glm::vec3 diff_coeff = { 2.0f, 2.0f, 2.0f }; // color
        glm::vec3 spec_coeff = { 0.5f, 0.5f, 0.5f }; // shine color
        float spec_exponent = 5; // amount of shine
        bool selected = false;


        void OnImGuiRender(Gep::EngineManager& em)
        {
        }
    };
}
