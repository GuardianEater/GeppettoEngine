/*****************************************************************//**
 * \file   SoundSystem.cpp
 * \brief  implementation of the SoundSystem class
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
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

        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<SpatialSoundEmitter>>(this, &SoundSystem::OnSpatialSoundEmitterAdded);
        mManager.SubscribeToEvent<Gep::Event::ComponentRemoved<SpatialSoundEmitter>>(this, &SoundSystem::OnSpatialSoundEmitterRemoved);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<SpatialSoundEmitter>>(this, &SoundSystem::OnSpatialSoundEmitterEditorRender);

        soundResource.mSoundEngine.init();
        soundResource.mSoundEngine.setGlobalVolume(1.0f);
    }

    void SoundSystem::Update(float dt)
    {
        mManager.GetResource<SoundResource>().mSoundEngine.update3dAudio();
        SoundResource& soundResource = mManager.GetResource<SoundResource>();

        const std::vector<Gep::Entity>& entities = mManager.GetEntities<SpatialSoundEmitter, Transform>();
        const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Camera, Transform>();

        for (const Gep::Entity camera : cameras)
        {
            Transform& cameraTransform = mManager.GetComponent<Transform>(camera);
            Camera& cameraComponent = mManager.GetComponent<Camera>(camera);

            soundResource.mSoundEngine.set3dListenerPosition(
                cameraTransform.world.position.x,
                cameraTransform.world.position.y,
                cameraTransform.world.position.z);
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
                SpatialSoundEmitter& soundComponent = mManager.GetComponent<SpatialSoundEmitter>(entity);
                Transform& transform = mManager.GetComponent<Transform>(entity);

                if (!soundResource.mSoundEngine.isValidVoiceHandle(soundComponent.soundHandle))
                    continue;

                soundResource.mSoundEngine.set3dSourcePosition(
                    soundComponent.soundHandle,
                    transform.world.position.x,
                    transform.world.position.y,
                    transform.world.position.z);

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

    void SoundSystem::OnSpatialSoundEmitterAdded(const Gep::Event::ComponentAdded<SpatialSoundEmitter>& event)
    {

    }

    void SoundSystem::OnSpatialSoundEmitterRemoved(const Gep::Event::ComponentRemoved<SpatialSoundEmitter>& event)
    {
        mManager.GetResource<SoundResource>().mSoundEngine.stop(event.component.soundHandle);
    }

    void SoundSystem::OnSpatialSoundEmitterEditorRender(const Gep::Event::ComponentEditorRender<SpatialSoundEmitter>& event)
    {
        SpatialSoundEmitter& soundEmitter = *event.components[0];

        SoundResource& sr = mManager.GetResource<Client::SoundResource>();
        EditorResource& er = mManager.GetResource<Client::EditorResource>();
        std::vector<std::filesystem::path> loadedSounds = sr.GetLoadedSounds();

        // drop down for selecting a mesh
        bool soundsOpen = ImGui::BeginCombo("Sound", soundEmitter.soundPath.string().c_str());

        er.AssetBrowserDropTarget({ ".ogg", ".mp3", ".wav" }, [&](const std::filesystem::path& droppedPath)
            {
                if (!sr.IsSoundLoaded(droppedPath))
                {
                    sr.LoadSound(droppedPath);
                }

                soundEmitter.soundPath = droppedPath;
            });

        if (soundsOpen)
        {
            for (const std::filesystem::path& loadedSound : loadedSounds)
            {
                bool isSelected = loadedSound == soundEmitter.soundPath;
                if (ImGui::Selectable(loadedSound.string().c_str(), isSelected))
                {
                    soundEmitter.soundPath = loadedSound;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (!sr.IsSoundLoaded(soundEmitter.soundPath))
        {
            return;
        }

        SoLoud::Soloud& soundEngine = sr.mSoundEngine;
        SoLoud::Wav& soundSource = sr.GetSound(soundEmitter.soundPath);

        if (ImGui::Checkbox("Looping", &soundEmitter.looping))
        {
            if (soundEngine.isValidVoiceHandle(soundEmitter.soundHandle))
            {
                soundEngine.setLooping(soundEmitter.soundHandle, soundEmitter.looping);
            }
        }

        float displayVolume = soundEmitter.volume * 100.0f;
        if (ImGui::DragFloat("Volume", &displayVolume, 1.0f, 0.0f, 100.0f, "%.0f%%"))
        {
            soundEmitter.volume = displayVolume / 100.0f;
            if (soundEngine.isValidVoiceHandle(soundEmitter.soundHandle))
            {
                soundEngine.setVolume(soundEmitter.soundHandle, soundEmitter.volume);
            }
        }

        ImGui::DragFloat("Distance", &soundEmitter.distance, 0.01f, 0.0f, 100.0f);

        std::string display = "Play";
        if (soundEngine.isValidVoiceHandle(soundEmitter.soundHandle))
        {
            if (soundEngine.getPause(soundEmitter.soundHandle))
            {
                display = "Resume";
            }
            else
            {
                display = "Pause";
            }
        }

        if (ImGui::Button(display.c_str()))
        {
            if (soundEngine.isValidVoiceHandle(soundEmitter.soundHandle))
            {
                if (soundEngine.getPause(soundEmitter.soundHandle))
                {
                    soundEngine.setPause(soundEmitter.soundHandle, false);
                }
                else
                {
                    soundEngine.setPause(soundEmitter.soundHandle, true);
                }
            }
            else
            {
                soundEmitter.soundHandle = soundEngine.play3d(soundSource, 0, 0, 0);
                soundEngine.setLooping(soundEmitter.soundHandle, soundEmitter.looping);
                soundEngine.setVolume(soundEmitter.soundHandle, soundEmitter.volume);
                soundEmitter.endTime = soundSource.getLength();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Restart"))
        {
            if (soundEngine.isValidVoiceHandle(soundEmitter.soundHandle))
            {
                soundEngine.setPause(soundEmitter.soundHandle, false);
                soundEngine.stop(soundEmitter.soundHandle);
                soundEmitter.currentTime = 0.0f;
            }
        }

        if (soundEngine.isValidVoiceHandle(soundEmitter.soundHandle))
        {
            soundEmitter.currentTime = soundEngine.getStreamPosition(soundEmitter.soundHandle);
            size_t loopCount = soundEngine.getLoopCount(soundEmitter.soundHandle);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1)); // Reduce vertical padding
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 5.0f); // Set grab size to 5px
        //ImGui::ProgressBar(currentTime / endTime, ImVec2(-FLT_MIN, 8), "");
        if (ImGui::SliderFloat("##Time", &soundEmitter.currentTime, 0.0f, soundEmitter.endTime, ""))
        {
            if (soundEngine.isValidVoiceHandle(soundEmitter.soundHandle))
            {
                soundEngine.seek(soundEmitter.soundHandle, soundEmitter.currentTime);
            }
        }
        ImGui::PopStyleVar(2);

        ImGui::Text("%.2f / %.2f", soundEmitter.currentTime, soundEmitter.endTime);
    }
}
