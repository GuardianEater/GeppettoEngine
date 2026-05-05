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
    template <typename ComponentType> struct ComponentSerializing;
    template <typename ComponentType> struct ComponentDeserializing;
}

// fwd
namespace Client
{
    struct SkeletonComponent;
    struct ModelComponent;
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
        void OnSkeletonAdded(const Gep::Event::ComponentAdded<SkeletonComponent>& event);
        void OnStaticModelSerializing(const Gep::Event::ComponentSerializing<ModelComponent>& event);
        void OnStaticModelDeserializing(const Gep::Event::ComponentDeserializing<ModelComponent>& event);

        // on editor render
        void OnModelEditorRender(const Gep::Event::ComponentEditorRender<ModelComponent>& event);
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
        void InitializeSkeleton(SkeletonComponent& modelComponent, const Gep::Skeleton& internalModel);

        void ImGuiUpdate();

        // resources
        Gep::OpenGLRenderer& mRenderer;
        Client::CollisionResource& mCollisionResource;
        Client::EditorResource& mEditorResource;
    };
}


