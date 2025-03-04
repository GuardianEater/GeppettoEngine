/*****************************************************************//**
 * \file   SerializationResource.hpp
 * \brief  SerializationResource is a resource that can assists in saving and loading the state of the engine
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"

namespace Client
{
    class SerializationResource
    {
    public:
        void SaveScene(const std::filesystem::path& path);
        void LoadScene(const std::filesystem::path& path);
    };
}
