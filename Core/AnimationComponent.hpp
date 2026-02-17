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
        uint64_t animationID = Gep::NumMax<uint64_t>(); // the id of the animation that this component is referencing, used for quick access to the animation data in the renderer
        float currentTime = 0.0f; // note this is stored in ticks
        bool looping = false; // whether or not the animation will loop
        float speed = 1.0f; // the playback multiplier of the animation
		float speedModifier = 1.0f; // current velocity of the animation in ticks per second
    };
}
