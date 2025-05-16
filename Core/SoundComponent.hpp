/*****************************************************************//**
 * \file   SpatialSoundEmitter.cpp
 * \brief  component that holds sound data
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "SoundResource.hpp"
#include "EditorResource.hpp"

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
    };
}