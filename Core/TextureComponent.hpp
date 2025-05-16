/*****************************************************************//**
 * \file   TextureComponent.hpp
 * \brief  adds a texture to an entity
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include <string>

namespace Client
{
    struct Texture
    {
        std::filesystem::path texturePath = "assets\\textures\\Checker.jpg";

        void OnImGuiRender(Gep::EngineManager& em)
        {
            
        }
    };
}
