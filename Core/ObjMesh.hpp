/*****************************************************************//**
 * \file   ObjMesh.hpp
 * \brief  loads an obj file into a mesh
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "Mesh.hpp"

namespace Gep
{
    Mesh LoadObjMesh(const std::filesystem::path& objPath);
}
