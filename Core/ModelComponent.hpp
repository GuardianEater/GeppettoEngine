/*****************************************************************//**
 * \file   MeshComponenet.hpp
 * \brief  Component for storing material data such as color or texture
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <glm\glm.hpp>
#include <EngineManager.hpp>

namespace Client
{
    // this model variant is used with models that have rigging or armature. Needed for animations or IK
    struct SkeletonComponent
    {
        std::vector<Gep::VQS> pose; // the bone offsets of the current model 
    };

    // high performance model component that will simply render the model
    struct ModelComponent
    {
        uint64_t modelIdx = 2; // cube
        std::vector<uint32_t> materialOverrides;
        bool selected = false;
    };
}
