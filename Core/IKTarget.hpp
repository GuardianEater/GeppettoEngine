// ik target component
// makes the entity a target for inverse kinematics

#pragma once

#include "UUID.hpp"

#include <limits>

#undef max

namespace Client
{
    // this component works backwards, it gets a target entity with a model, and pulls the target bone towards this entity
    struct IKTarget
    {
        Gep::UUID targetEntity;
        uint32_t endBone = std::numeric_limits<uint32_t>::max(); // this is the restriction bone so the whole model doesn't move (ie shoulder)
        uint32_t startBone = std::numeric_limits<uint32_t>::max(); // this is the bone that will end on top of the target (ie hand)
    };
}