/*****************************************************************//**
 * \file   ChildComponent.hpp
 * \brief  component the defines an entity as a child
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include "Tags.hpp"
#include "Transform.hpp"

namespace Client
{
    struct Child
    {
        Gep::Entity parent{ Gep::INVALID_ENTITY };

        Transform offset;
    };
}
