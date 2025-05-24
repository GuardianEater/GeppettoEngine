/*****************************************************************//**
 * \file   SkyboxMesh.hpp
 * \brief  a cube mesh with inverted normals, and special uvs.
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include "Mesh.hpp"

namespace Gep
{
    struct SkyboxMesh : public Gep::Mesh
    {
        SkyboxMesh();
    };
}
