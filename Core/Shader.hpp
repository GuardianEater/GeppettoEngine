/*****************************************************************//**
 * \file   Shader.hpp
 * \brief  structure to help with shaders
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include "glm/fwd.hpp"
#include <glew.h>

namespace Gep
{
    class Shader
    {
    public:
        static Shader FromFile(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath, const std::filesystem::path& geomPath = ""); // reads shaders in from files, supports includes
        static Shader FromSource(const std::string& vertSrc, const std::string& fragSrc, const std::string& geomSrc = ""); // reads shader in from source, this DOES NOT support includes

        Shader() = default;
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;

        bool IsValid() const; // checks if the shader compiled successfully

        void SetUniform(const std::string& name, const glm::vec3& v);
        void SetUniform(const std::string& name, const glm::vec4& v);
        void SetUniform(const std::string& name, const glm::mat4& v, bool transpose = false);
        void SetUniform(const std::string& name, int v);
        void SetUniform(const std::string& name, float v);
        void SetUniform(const std::string& name, uint32_t v);
        void SetUniform(const std::string& name, uint64_t v);

        void SetUniform(size_t location, const glm::vec3& v);
        void SetUniform(size_t location, const glm::vec4& v);
        void SetUniform(size_t location, const glm::mat4& v, bool transpose = false);
        void SetUniform(size_t location, int v);
        void SetUniform(size_t location, float v);
        void SetUniform(size_t location, uint32_t v);
        void SetUniform(size_t location, uint64_t v);

        // recompiles the shader
        void Reload();

        void Bind();
        static void Unbind();

    private:
        static GLuint CreateProgram(GLuint vertexShader, GLuint fragmentShader, GLuint geometryShader, const std::string& origin = "<embedded>");

    private:
        // whenever an include is evaluated it is added to this map to prevent redundant loads (path -> source) emptied when any shader is reloaded
        GLuint mProgram = 0;

        // if FromFile
        std::filesystem::path mVertPath{};
        std::filesystem::path mFragPath{};
        std::filesystem::path mGeomPath{};
    };

    class ComputeShader
    {
    public:
        static ComputeShader FromFile(const std::filesystem::path& compPath); // reads shaders in from files, supports includes
        static ComputeShader FromSource(const std::string& compSrc); // reads shader in from source, this DOES NOT support includes

        ComputeShader() = default;
        ~ComputeShader();

        ComputeShader(const ComputeShader&) = delete;
        ComputeShader& operator=(const ComputeShader&) = delete;

        ComputeShader(ComputeShader&& other) noexcept;
        ComputeShader& operator=(ComputeShader&& other) noexcept;

        // will automatically divide amongst work groups. this function takes the original texture size
        void Dispatch(glm::uvec3 targetTextureSize);

        bool IsValid() const;

        void Bind();
        void Unbind();


    private:
        static GLuint CreateProgram(GLuint computeShader, const std::string& origin = "<embedded>");

    private:
        glm::ivec3 mWorkGroupSize{0};
        GLuint mProgram = 0;
        std::filesystem::path mCompPath{};
    };
}
