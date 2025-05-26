/*****************************************************************//**
 * \file   FileHelp.cpp
 * \brief  implementation for filesystem helpers
 *
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#include "pch.hpp"
#include "FileHelp.hpp"

namespace Gep
{
    std::filesystem::path UniqueFileName(const std::filesystem::path& path)
    {
        const std::filesystem::path directory = path.parent_path();
        const std::string ext = path.extension().string();
        const std::string originalFilename = path.stem().string();

        std::filesystem::path result;
        std::string filename = originalFilename;
        size_t scenePathNumber = 1;

        result = directory / (filename + ext);
        while (std::filesystem::exists(result))
        {
            filename = originalFilename + " (" + std::to_string(scenePathNumber) + ")";
            result = directory / (filename + ext);
        }

        return result;
    }

    std::string ReadFile(const std::filesystem::path& path)
    {
        std::ifstream in(path);

        if (!in) return "";

        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
}

