/*****************************************************************//**
 * \file   AnimationSystem.hpp
 * \brief  causes models to animate
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   September 2025
 *********************************************************************/

#pragma once

#include "ISystem.hpp"
#include "Events.hpp"
#include "AnimationComponent.hpp"

namespace Gep
{
    class EngineManager;
    struct VQS;
    class OpenGLRenderer;
    struct LineGPUData;
    struct Animation;
}

namespace Client
{
    class AnimationSystem : public Gep::ISystem
    {
    public:
        AnimationSystem(Gep::EngineManager& em);

        void Initialize() override;
        void Update(float dt) override;

    private:
        void EvaluateAnimation(const Gep::Animation& animation, float time, std::vector<Gep::VQS>& outLocalPose);
        void OnAnimationEditorRender(const Gep::Event::ComponentEditorRender<AnimationComponent>& event);

    private:
        Gep::OpenGLRenderer& mRenderer;
    };
}
