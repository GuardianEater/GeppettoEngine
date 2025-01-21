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
#include "SphereMesh.hpp"
#include "CubeMesh.hpp"

// client
#include "Transform.hpp"
#include "Material.hpp"
#include "CameraComponent.hpp"
#include "TextureComponent.hpp"

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
        void KeyEvent(const Gep::Event::KeyPressed& eventData);

    private:
        Gep::IRenderer mRenderer;
    };
}


