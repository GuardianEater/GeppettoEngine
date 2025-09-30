/*****************************************************************//**
 * \file   AnimationSystem.hpp
 * \brief  causes models to animate
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   September 2025
 *********************************************************************/

#pragma once

#include "ISystem.hpp"

namespace Gep
{
    class EngineManager;
    class OpenGLRenderer;
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

    private:
        Gep::OpenGLRenderer& mRenderer;
    };
}
