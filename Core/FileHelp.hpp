/*****************************************************************//**
 * \file   FileHelp.hpp
 * \brief  useful function for when working with files
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include <filesystem>

namespace Gep
{
    // given a file path, if it already exists on disk will make it unique
    std::filesystem::path UniqueFileName(const std::filesystem::path& path);
}
