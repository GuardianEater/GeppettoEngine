/*****************************************************************//**
 * \file   RenderSystem.hpp
 * \brief  System that renders objects
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

 // core
#include <Core.hpp>

// backend
#include "ISystem.hpp"
#include "EngineManager.hpp"
#include "Renderer.hpp"

// Meshes
#include "SphereMesh.hpp"
#include "CubeMesh.hpp"
#include "IcosphereMesh.hpp"
#include "QuadMesh.hpp"

// client
#include "Transform.hpp"
#include "Material.hpp"
#include "CameraComponent.hpp"
#include "TextureComponent.hpp"
#include "LightComponent.hpp"

namespace Client
{
    class RenderSystem : public Gep::ISystem
    {
    public:
        RenderSystem(Gep::EngineManager& em);
        ~RenderSystem();

        void Initialize();
        void Update(float dt);
        void HandleInputs(float dt);
        void RenderImGui(float dt);
    };
}


