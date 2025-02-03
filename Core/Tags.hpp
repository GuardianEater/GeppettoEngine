/*****************************************************************//**
 * \file   Tags.hpp
 * \brief  generic modifiers for components to change how they are handled
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include <type_traits>
#include <TypeList.hpp>

 // Example:
// struct Transform
// {
//     using Tags = Gep::Tags<Gep::Tag::DontShowInEditor>;
// 
//     glm::vec3 position{};
//     glm::vec3 scale{1.0f, 1.0f, 1.0f};
//     glm::vec3 rotation{};
// };

namespace Gep
{
    namespace Tag
    {
        template <typename... TagTypes>
        using TagList = Gep::type_list<TagTypes...>;

        using DontShowInEditor = std::true_type;
        using DontSerialize    = std::true_type;
        using DontShowInScipts = std::true_type;
    }
}
