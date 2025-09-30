/*****************************************************************//**
 * \file   AnimationComponent.hpp
 * \brief  contains information for this individual entity to animate
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   September 2025
 *********************************************************************/

#pragma once

#include <string>

namespace Client
{
    struct AnimationComponent
    {
        std::string name = "Armature|Armature|mixamo.com|Layer0";
        float currentTime = 0.0f;
        bool looping = false;
        float speed = 1.0f; 
    };
}
