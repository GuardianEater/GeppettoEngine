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
        glm::vec3 color = { 1.0f, 1.0f, 1.0f }; // color; both diffuse and specular coefficient in phong
        glm::vec3 spec_coeff = { 0.0f, 0.0f, 0.0f }; // shine color
        float spec_exponent = 1.0f; // amount of shine
        bool selected = false;
        bool ignoreLight = false;
    };
}
