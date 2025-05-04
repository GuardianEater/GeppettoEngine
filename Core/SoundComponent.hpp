/*****************************************************************//**
 * \file   SpatialSoundEmitter.cpp
 * \brief  component that holds sound data
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "SoundResource.hpp"

namespace Client
{
    struct SpatialSoundEmitter
    {
        bool looping = false;
        float volume = 1.0f; // percent
        float currentTime = 0.0f;
        float endTime = 1.0f;
        float distance = 1.0f;
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
            
            float displayVolume = volume * 100.0f;
            if (ImGui::DragFloat("Volume", &displayVolume, 1.0f, 0.0f, 100.0f, "%.0f%%"))
            {
                volume = displayVolume / 100.0f;
                if (soundEngine.isValidVoiceHandle(soundHandle))
                {
                    soundEngine.setVolume(soundHandle, volume);
                }
            }

            ImGui::DragFloat("Distance", &distance, 0.01f, 0.0f, 100.0f);

            std::string display = "Play";
            if (soundEngine.isValidVoiceHandle(soundHandle))
            {
                if (soundEngine.getPause(soundHandle))
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
                if (soundEngine.isValidVoiceHandle(soundHandle))
                {
                    if (soundEngine.getPause(soundHandle))
                    {
                        soundEngine.setPause(soundHandle, false);
                    }
                    else
                    {
                        soundEngine.setPause(soundHandle, true);
                    }
                }
                else
                {
                    soundHandle = soundEngine.play3d(soundSource, 0, 0, 0);
                    soundEngine.setLooping(soundHandle, looping);
                    soundEngine.setVolume(soundHandle, volume);
                    endTime = soundSource.getLength();
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Restart"))
            {
                if (soundEngine.isValidVoiceHandle(soundHandle))
                {
                    soundEngine.setPause(soundHandle, false);
                    soundEngine.stop(soundHandle);
                    currentTime = 0.0f;
                }
            }

            if (soundEngine.isValidVoiceHandle(soundHandle))
            {
                currentTime = soundEngine.getStreamPosition(soundHandle);
                size_t loopCount = soundEngine.getLoopCount(soundHandle);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1)); // Reduce vertical padding
            ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 5.0f); // Set grab size to 5px
            //ImGui::ProgressBar(currentTime / endTime, ImVec2(-FLT_MIN, 8), "");
            if (ImGui::SliderFloat("##Time", &currentTime, 0.0f, endTime, ""))
            {
                if (soundEngine.isValidVoiceHandle(soundHandle))
                {
                    soundEngine.seek(soundHandle, currentTime);
                }
            }
            ImGui::PopStyleVar(2);

            ImGui::Text("%.2f / %.2f", currentTime, endTime);
        }
    };
}