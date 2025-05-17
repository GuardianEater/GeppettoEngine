/*****************************************************************//**
 * \file   RenderTargetImgui.hpp
 * \brief  renders to an imgui window
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include "IRenderTarget.hpp"
#include "imgui.h"
#include "RenderTargetTexture.hpp"

namespace Client
{
    struct Transform;
}

namespace Gep
{
    class RenderTargetImgui : public RenderTargetTexture
    {
    public:
        RenderTargetImgui(int width, int height)
            : RenderTargetTexture(width, height)
        {}

        void Draw(EngineManager& em, Entity camera, const std::function<void()>& drawFunction = []() {}) override;

    private:

        struct EntityTransformPair
        {
            Entity entity;
            Client::Transform& transform;
        };

        void HandleGuizmo(EngineManager& em);
        void StartCameraFocus(EngineManager& em, Gep::Entity camera, const glm::vec3& targetPosition, float targetScale);
        void UpdateCameraFocus(EngineManager& em, Gep::Entity entity, float dt);
        float ComputeContainingScale(const std::vector<EntityTransformPair>& etps, const glm::vec3& avgPos);
        glm::vec3 mCameraTargetPosition{};
        glm::vec3 mCameraTargetRotation{};
        bool mCameraLerping = false;
    };
}
