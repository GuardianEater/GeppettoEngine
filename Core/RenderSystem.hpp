/*****************************************************************//**
 * \file   RenderSystem.hpp
 * \brief  System that renders objects
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include "ISystem.hpp"

// fwd
namespace Gep
{
    class EngineManager;
    class OpenGLRenderer;

    struct Skeleton;
    struct VQS;
    struct LineGPUData;
}

// fwd
namespace Gep::Event
{
    template <typename ComponentType> struct ComponentAdded;
    template <typename ComponentType> struct ComponentEditorRender;
}

// fwd
namespace Client
{
    struct ModelComponent;
    struct Texture;
    struct Light;
    struct Camera;
}

// client
namespace Client
{
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
        // flags
        bool mDrawColliders = false;
        bool mWireframeMode = false;
        bool mNoTextureMode = false;
        bool mDrawBones     = false;
        bool mDrawAABBs     = false;

        // events
        void OnModelAdded(const Gep::Event::ComponentAdded<ModelComponent>& event);
        void OnModelEditorRender(const Gep::Event::ComponentEditorRender<ModelComponent>& event);
        void OnTextureEditorRender(const Gep::Event::ComponentEditorRender<Texture>& event);
        void OnLightEditorRender(const Gep::Event::ComponentEditorRender<Light>& event);
        void OnCameraEditorRender(const Gep::Event::ComponentEditorRender<Camera>& event);

        // helpers
        void AddColliders();
        void AddLights();
        void AddCameras();
        void AddObjects();

        // resources
        Gep::OpenGLRenderer& mRenderer;
    };
}


