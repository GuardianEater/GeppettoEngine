/*****************************************************************//**
 * \file   Transform.hpp
 * \brief  transform component, stores position data
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

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
