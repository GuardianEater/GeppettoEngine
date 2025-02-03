/*****************************************************************//**
 * \file   ParentComponent.hpp
 * \brief  This component defines an entity as a parent
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include "Tags.hpp"

namespace Client
{
    struct Parent
    {
        using Tags = Gep::Tag::TagList<Gep::Tag::DontShowInEditor, Gep::Tag::DontSerialize>;

        std::vector<Gep::Entity> children{};
    };
}
