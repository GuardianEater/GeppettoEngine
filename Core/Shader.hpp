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
        Shader(const std::filesystem::path& vertexShader, const std::filesystem::path& fragmentShader);

        bool IsValid() const; // checks if the shader compiled successfully

        void SetUniform(const std::string& name, const glm::vec3& v);
        void SetUniform(const std::string& name, const glm::vec4& v);
        void SetUniform(const std::string& name, const glm::mat4& v, bool transpose = false);
        void SetUniform(const std::string& name, int v);
        void SetUniform(const std::string& name, float v);

        // during the scope of the lamda the shader is bound
        template <typename Func>
        void Use(Func&& func);

    private:
        GLuint Compile(GLenum shaderType, const std::string& source) const;
        GLuint CreateProgram(GLuint vertexShader, GLuint fragmentShader) const;

        void Bind();
        void Unbind();

    private:
        GLuint mProgram = 0;
        GLint mPrevious = 0; // used to preserve a stack, restores whatever program was active on unbind
    };

    template<typename Func>
    inline void Shader::Use(Func&& func)
    {
        Bind();
        func();
        Unbind();
    }
}
