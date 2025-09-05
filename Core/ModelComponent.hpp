/*****************************************************************//**
 * \file   MeshComponenet.hpp
 * \brief  Component for storing material data such as color or texture
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <glm\glm.hpp>
#include <EngineManager.hpp>
#include <imgui.h>
#include "OpenGLRenderer.hpp"
#include "EditorResource.hpp"

namespace Client
{
    struct ModelComponent
    {
        std::string meshName{ "Cube" };
        bool selected = false;
        bool ignoreLight = false;

        float ao = 0.8f;
        float roughness = 0.8f;
        float metalness = 0.8f;
        glm::vec3 color = { 0.5f, 1.0f, 1.0f }; // color; both diffuse and specular coefficient in phong

        GLuint meshIndex = 0;
        GLuint materialIndex = 0;
        GLuint shaderIndex = 0;
    };
}
