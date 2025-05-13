/*****************************************************************//**
 * \file   ModelResource.hpp
 * \brief  imports models into the engine
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include "EngineManager.hpp"

namespace Client
{
    class ModelResource
    {
    public:

        // contructs an entity given the 
        Gep::Entity ImportModel(Gep::EngineManager& em, const std::filesystem::path& path)
        {
            return Gep::INVALID_ENTITY;
        }

    private:
    };
}
