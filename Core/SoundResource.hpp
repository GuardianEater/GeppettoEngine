/*****************************************************************//**
 * \file   SoundResource.hpp
 * \brief  stores sound data
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include <soloud.h>
#include <soloud_wav.h>

namespace Client
{
    class SoundResource
    {
    private:
        std::unordered_map<std::filesystem::path, SoLoud::Wav> mSounds;// given the name of a sound will return the audio source for the sound

    public:
        void LoadSound(const std::filesystem::path& path)
        {
            if (mSounds.contains(path))
            {
                Gep::Log::Error("Sound already loaded: ", path);
                return;
            }

            SoLoud::Wav& newSound = mSounds[path];
            SoLoud::result result = newSound.load(path.string().c_str());

            if (result != SoLoud::SO_NO_ERROR)
            {
                Gep::Log::Error("Unsupported sound format: ", path);
                mSounds.erase(path);
            }
        }

        SoLoud::Wav& GetSound(const std::filesystem::path& path)
        {
            return mSounds.at(path);
        }

        std::vector<std::filesystem::path> GetLoadedSounds() const
        {
            std::vector<std::filesystem::path> names;
            for (const auto& [name, _] : mSounds)
            {
                names.push_back(name);
            }
            return names;
        }

        bool IsSoundLoaded(const std::filesystem::path& path) const
        {
            return mSounds.contains(path);
        }

        SoLoud::Soloud mSoundEngine;
    };
}
