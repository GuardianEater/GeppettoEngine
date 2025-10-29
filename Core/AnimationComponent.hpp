/*****************************************************************//**
 * \file   AnimationComponent.hpp
 * \brief  contains information for this individual entity to animate
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   September 2025
 *********************************************************************/

#pragma once

#include <string>

namespace Gep
{
    struct VQS;
}

namespace Client
{
    struct AnimationComponent
    {
        std::string name = ""; // the name of the animation that this component is referencing
        float currentTime = 0.0f; // note this is stored in ticks
        bool looping = false; // whether or not the animation will loop
        float speed = 1.0f; // the playback multiplier of the animation

        std::vector<Gep::VQS> pose; // the current pose of the animation, in global space
    };
}
