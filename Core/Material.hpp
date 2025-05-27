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
        std::string shaderName{ "PBR" };
        glm::vec3 spec_coeff = { 0.0f, 0.0f, 0.0f }; // shine color
        float spec_exponent = 1.0f; // amount of shine
        bool selected = false;
        bool ignoreLight = false;

        float ao = 0.8f;
        float roughness = 0.8f;
        float metalness = 0.8f;
        glm::vec3 color = { 0.5f, 1.0f, 1.0f }; // color; both diffuse and specular coefficient in phong
    };
}
