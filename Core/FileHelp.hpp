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

    // reads a file from the given path into a string, will silently fail and return "" if the path doesnt exist
    std::string ReadFile(const std::filesystem::path& path);

    // takes an opengl texture and writes it to a png file
    void WritePNG(const std::filesystem::path& path, int width, int height, GLuint texID);
}
