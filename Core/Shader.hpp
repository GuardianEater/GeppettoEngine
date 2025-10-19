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
        static Shader FromFile(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath); // reads shaders in from files, supports includes
        static Shader FromSource(const std::string& vertSrc, const std::string& fragSrc); // reads shader in from source, this DOES NOT support includes

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

        void Bind();
        static void Unbind();
        
        template <typename Func>
        void Use(Func&& func); // during the scope of the lamda the shader is bound

    private:
        friend class Renderer;

    private:
        static GLuint Compile(GLenum shaderType, const std::string& source);
        static GLuint CreateProgram(GLuint vertexShader, GLuint fragmentShader);
        static std::string ReadShader(const std::filesystem::path& path); // reads in the data and handles includes
        
    private:
        GLuint mProgram = 0;
    };

    template<typename Func>
    inline void Shader::Use(Func&& func)
    {
        Bind();
        func();
        Unbind();
    }
}
