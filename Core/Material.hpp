/*****************************************************************//**
 * \file   Material.hpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>

namespace Gep
{
    // contains material data for pbr rendering.
    struct Material
    {
        // if it has a uniform material across the entire mesh
        float ao; // ambient occlusion
        float roughness;
        float metalness;
        glm::vec3 color;

        bool hasAoTexture = false;
        GLuint aoTextureHandle;

        bool hasRoughnessTexture = false;
        GLuint roughnessTextureHandle;

        bool hasMetalnessTexture = false;
        GLuint metalnessTextureHandle;

        bool hasDiffuseTexture = false;
        GLuint diffuseTextureHandle;
    };
}
