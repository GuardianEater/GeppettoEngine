/*********************************************************************
 * file:   BoneComponent.hpp
 * author: travis.gronvold (travis.gronvold@digipen.edu)
 * date:   November 13, 2025
 * Copyright © 2023 DigiPen (USA) Corporation. 
 * 
 * brief:  Makes an entity identify as a bone in a model
 *********************************************************************/

#pragma once

#include "VQS.hpp"

namespace Client
{
    struct Bone
    {
        Gep::VQS inverseBind{};
        bool isRealBone = false; // wether or not this bone was a bone inside of assimp
    };
}
