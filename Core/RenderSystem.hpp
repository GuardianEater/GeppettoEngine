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
    struct Model;
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
    struct RiggedModelComponent;
    struct StaticModelComponent;
    struct Texture;
    struct Light;
    struct Camera;
    struct Transform;
    struct DirectionalLight;
    struct CollisionResource;
    struct EditorResource;
    struct ShadowCasterComponent;
}

// client
namespace Client
{
    class RenderSystem : public Gep::ISystem
    {
    public:
        RenderSystem(Gep::EngineManager& em);
        ~RenderSystem();

        void Initialize() override;
        void Update(float dt) override;
        void FrameEnd() override;
        void HandleInputs(float dt);

    private:
        // flags ///////////
        bool mDrawColliders = false;
        bool mWireframeMode = false;
        bool mNoTextureMode = false;
        bool mDrawBones     = false;
        bool mDrawAABBs     = false;

        // events //////////
        // on added
        void OnRiggedModelAdded(const Gep::Event::ComponentAdded<RiggedModelComponent>& event);
        void OnStaticModelAdded(const Gep::Event::ComponentAdded<StaticModelComponent>& event);

        // on editor render
        void OnRiggedModelEditorRender(const Gep::Event::ComponentEditorRender<RiggedModelComponent>& event);
        void OnStaticModelEditorRender(const Gep::Event::ComponentEditorRender<StaticModelComponent>& event);
        void OnPointLightEditorRender(const Gep::Event::ComponentEditorRender<Light>& event);
        void OnShadowCasterEditorRender(const Gep::Event::ComponentEditorRender<ShadowCasterComponent>& event);
        void OnDirectionalLightEditorRender(const Gep::Event::ComponentEditorRender<DirectionalLight>& event);
        void OnCameraEditorRender(const Gep::Event::ComponentEditorRender<Camera>& event);

        void DrawImGuiCameraWindow(Gep::Entity cameraEntity, Client::Camera& camera, Client::Transform& cameraTransform);

        // helpers /////////
        void AddColliders();
        void AddLights();
        void AddCameras();
        void AddObjects();

        // when a model is changed 
        void InitializeModelPose(RiggedModelComponent& modelComponent, const Gep::Model& internalModel);

        // resources
        Gep::OpenGLRenderer& mRenderer;
        Client::CollisionResource& mCollisionResource;
        Client::EditorResource& mEditorResource;
    };
}


