/*****************************************************************//**
 * \file   SoundComponent.cpp
 * \brief  component that holds sound data
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "SoundResource.hpp"

namespace Client
{
    struct SoundComponent
    {
        bool looping = false;
        float volume = 1.0f;
        float currentTime = 0.0f;
        float endTime = 1.0f;
        SoLoud::handle soundHandle;

        std::filesystem::path soundPath;

        void OnImGuiRender(Gep::EngineManager& em)
        {
            SoundResource& sr = em.GetResource<Client::SoundResource>();
            std::vector<std::filesystem::path> loadedSounds = sr.GetLoadedSounds();

            // drop down for selecting a mesh
            bool soundsOpen = ImGui::BeginCombo("Sound", soundPath.string().c_str());

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(char) * (strlen((const char*)payload->Data) + 1));
                    const char* path = (const char*)payload->Data;
                    std::filesystem::path droppedPath(path);

                    if (!sr.IsSoundLoaded(droppedPath))
                    {
                        sr.LoadSound(droppedPath);
                    }

                    soundPath = droppedPath;
                }
                ImGui::EndDragDropTarget();
            }

            if (soundsOpen)
            {
                for (const std::filesystem::path& loadedSound : loadedSounds)
                {
                    bool isSelected = loadedSound == soundPath;
                    if (ImGui::Selectable(loadedSound.string().c_str(), isSelected))
                    {
                        soundPath = loadedSound;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (!sr.IsSoundLoaded(soundPath))
            {
                ImGui::End();
                return;
            }

            SoLoud::Soloud& soundEngine = sr.mSoundEngine;
            SoLoud::Wav& soundSource = sr.GetSound(soundPath);

            if (ImGui::Checkbox("Looping", &looping))
            {
                if (soundEngine.isValidVoiceHandle(soundHandle))
                {
                    soundEngine.setLooping(soundHandle, looping);
                }
            }
            
            if (ImGui::DragFloat("Volume", &volume, 0.01f, 0.0f, 1.0f))
            {
                if (soundEngine.isValidVoiceHandle(soundHandle))
                {
                    soundEngine.setVolume(soundHandle, volume);
                }
            }

            if (ImGui::Button("Play"))
            {
                if (soundEngine.isValidVoiceHandle(soundHandle))
                {
                    if (soundEngine.getPause(soundHandle))
                    {
                        soundEngine.setPause(soundHandle, false);
                    }
                    else
                    {
                        soundEngine.stop(soundHandle);
                    }
                }
                else
                {
                    soundHandle = soundEngine.play(soundSource);
                    soundEngine.setLooping(soundHandle, looping);
                    soundEngine.setVolume(soundHandle, volume);

                    endTime = soundSource.getLength();
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Pause"))
            {
                if (soundEngine.isValidVoiceHandle(soundHandle))
                {
                    soundEngine.setPause(soundHandle, true);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Restart"))
            {
                if (soundEngine.isValidVoiceHandle(soundHandle))
                {
                    soundEngine.setPause(soundHandle, false);
                    soundEngine.stop(soundHandle);
                }
            }

            float displayTime = 0.0f;
            if (soundEngine.isValidVoiceHandle(soundHandle))
            {
                currentTime = soundEngine.getStreamTime(soundHandle);
                size_t loopCount = soundEngine.getLoopCount(soundHandle);

                displayTime = currentTime - (loopCount * endTime);
            }

            ImGui::ProgressBar(displayTime / endTime);
        }
    };
}