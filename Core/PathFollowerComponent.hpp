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
        Gep::UUID targetPathEntity{}; // the entity that is currently being followed

        std::pair<float, float> easeTimes = { 0.25f, 0.75f };
        double distanceAlongPath = 0.0; // distance along the target path in units
        double linearDistance = 0.0;
        float pace = 1.0f; // speed adjust to sync with animation
        float speed = 0.0f; // determines how fast to move down the path
        bool looping = false; // whether or not the entity should wrap to the other end of the path or stop
    };
}

