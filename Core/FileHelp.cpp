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

    void WritePNG(const std::filesystem::path& path, int width, int height, GLuint texID)
    {
        std::filesystem::create_directories(path.parent_path());

        std::vector<uint8_t> data(width * height * 4);
        glBindTexture(GL_TEXTURE_2D, texID);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_flip_vertically_on_write(1);
        stbi_write_png(path.string().c_str(), width, height, 4, data.data(), width * 4);
    }
}
