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
        std::string name{ "Cube" }; // name of the model that is currently in use
        bool selected = false;
        bool ignoreLight = false;

        std::vector<Gep::VQS> pose; // the bone offsets of the current model 
    };
}
