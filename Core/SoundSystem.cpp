/*****************************************************************//**
 * \file   SoundSystem.cpp
 * \brief  implementation of the SoundSystem class
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#include "pch.hpp"
#include "SoundSystem.hpp"
#include "SoundResource.hpp"

namespace Client
{
    void SoundSystem::Initialize()
    {
        mManager.GetResource<SoundResource>().mSoundEngine.init();
    }

    void SoundSystem::Update(float dt)
    {
        mManager.GetResource<SoundResource>().mSoundEngine.update3dAudio();
    }

    void SoundSystem::Exit()
    {
        mManager.GetResource<SoundResource>().mSoundEngine.deinit();
    }
}
