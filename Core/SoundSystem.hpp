/*****************************************************************//**
 * \file   SoundSystem.hpp
 * \brief  system that plays sounds
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "EngineManager.hpp"

namespace Client
{
    struct SpatialSoundEmitter;

    class SoundSystem : public Gep::ISystem
    {
    public:
        SoundSystem(Gep::EngineManager& em)
            : ISystem(em)
        {}

    private:
        void Initialize() override;
        void Update(float dt) override;
        void Exit() override;

        void OnSpatialSoundEmitterAdded(const Gep::Event::ComponentAdded<SpatialSoundEmitter>& event);
        void OnSpatialSoundEmitterRemoved(const Gep::Event::ComponentRemoved<SpatialSoundEmitter>& event);
    };
}
