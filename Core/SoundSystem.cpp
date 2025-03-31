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
#include "SoundComponent.hpp"
#include "Transform.hpp"
#include "CameraComponent.hpp"

namespace Client
{
    void SoundSystem::Initialize()
    {
        SoundResource& soundResource = mManager.GetResource<SoundResource>();

        soundResource.mSoundEngine.init();
        soundResource.mSoundEngine.setGlobalVolume(1.0f);
    }

    void SoundSystem::Update(float dt)
    {
        mManager.GetResource<SoundResource>().mSoundEngine.update3dAudio();
        SoundResource& soundResource = mManager.GetResource<SoundResource>();

        const std::vector<Gep::Entity>& entities = mManager.GetEntities<SoundComponent, Transform>();
        const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Camera, Transform>();

        for (const Gep::Entity camera : cameras)
        {
            Transform& cameraTransform = mManager.GetComponent<Transform>(camera);
            Camera& cameraComponent = mManager.GetComponent<Camera>(camera);

            soundResource.mSoundEngine.set3dListenerPosition(
                cameraTransform.position.x,
                cameraTransform.position.y,
                cameraTransform.position.z);
            soundResource.mSoundEngine.set3dListenerAt(
                -cameraComponent.back.x,
                -cameraComponent.back.y,
                -cameraComponent.back.z);
            soundResource.mSoundEngine.set3dListenerUp(
                cameraComponent.up.x,
                cameraComponent.up.y,
                cameraComponent.up.z);

            for (const Gep::Entity& entity : entities)
            {
                SoundComponent& soundComponent = mManager.GetComponent<SoundComponent>(entity);
                Transform& transform = mManager.GetComponent<Transform>(entity);

                if (!soundResource.mSoundEngine.isValidVoiceHandle(soundComponent.soundHandle))
                    continue;

                soundResource.mSoundEngine.set3dSourcePosition(
                    soundComponent.soundHandle,
                    transform.position.x,
                    transform.position.y,
                    transform.position.z);

                soundResource.mSoundEngine.set3dSourceMinMaxDistance(
                    soundComponent.soundHandle,
                    1.0f,
                    soundComponent.distance);

                soundResource.mSoundEngine.set3dSourceAttenuation(
                    soundComponent.soundHandle,
                    SoLoud::AudioSource::LINEAR_DISTANCE,
                    1.0f);
            }
        }

        soundResource.mSoundEngine.update3dAudio();
    }

    void SoundSystem::Exit()
    {
        mManager.GetResource<SoundResource>().mSoundEngine.deinit();
    }
}
