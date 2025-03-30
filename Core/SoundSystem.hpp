/*****************************************************************//**
 * \file   SoundSystem.hpp
 * \brief  system that plays sounds
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "EngineManager.hpp"

namespace Client
{
    class SoundSystem : public Gep::ISystem
    {
    public:
        SoundSystem(Gep::EngineManager& em)
            : ISystem(em)
        {}

        void Initialize() override;
        void Update(float dt) override;
        void Exit() override;
    };
}
