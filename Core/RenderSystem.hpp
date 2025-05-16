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
namespace Client
{
    struct Mesh;
    struct Texture;
    struct Light;
    struct Camera;

    class RenderSystem : public Gep::ISystem
    {
    public:
        RenderSystem(Gep::EngineManager& em);
        ~RenderSystem();

        void Initialize();
        void Update(float dt);
        void FrameEnd() override;
        void HandleInputs(float dt);
        void RenderImGui(float dt);

    private:
        bool mDrawColliders = false;
        void OnModelAdded(const Gep::Event::ComponentAdded<Mesh>& event);
        void OnMeshEditorRender(const Gep::Event::ComponentEditorRender<Mesh>& event);
        void OnTextureEditorRender(const Gep::Event::ComponentEditorRender<Texture>& event);
        void OnLightEditorRender(const Gep::Event::ComponentEditorRender<Light>& event);
        void OnCameraEditorRender(const Gep::Event::ComponentEditorRender<Camera>& event);

    };
}


