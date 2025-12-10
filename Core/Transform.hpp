/*****************************************************************//**
 * \file   Transform.hpp
 * \brief  transform component, stores position data
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <glm\glm.hpp>
#include "Affine.hpp"
#include "Conversion.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm\gtx\matrix_decompose.hpp>

#include "VQS.hpp"

namespace Client
{
    struct Transform
    {
        Gep::VQS local;
        Gep::VQS world;

        Gep::VQS previousWorld; // the word space transform from last frame, ie the transform 1 dt ago
    };
}
