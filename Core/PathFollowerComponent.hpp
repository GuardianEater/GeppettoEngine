/*****************************************************************//**
 * \file   PathFollowerComponent.hpp
 * \brief  component that references an entity with a Path component.
 *         this allows the entity to follow the given path
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   October 2025
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>

#include "UUID.hpp"

namespace Client
{
    struct PathFollowerComponent
    {
        Gep::UUID targetPathEntity{}; // the entity that is currently being followed, it must have a curve and a transform.

        std::pair<float, float> easeTimes = { 0.25f, 0.75f }; // ease start and ease end times

        double distanceAlongPath = 0.0; // visual distance along the target path in units
        double linearDistance = 0.0;    // effectively unnormalized t value. always increases and is not effected by easing
        float pace = 1.0f;              // have fast the object should move down the path. Effects animation playback speed.
        float speedAdjust = 0.0f;       // speed adjust to sync with animation
        bool looping = false;           // whether or not the entity should wrap to the other end of the path or stop
    };
}