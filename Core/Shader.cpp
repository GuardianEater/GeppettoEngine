/*****************************************************************//**
 * \file   Shader.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#include "pch.hpp"
#include "Shader.hpp"
#include "FileHelp.hpp"

namespace Gep
{
    Shader::Shader(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath)
    {
        std::string vertSource = Gep::ReadFile(vertPath);
        std::string fragSource = Gep::ReadFile(fragPath);

		GLuint vertShader = Compile(GL_VERTEX_SHADER, vertSource);
		GLuint fragShader = Compile(GL_FRAGMENT_SHADER, fragSource);

		mProgram = CreateProgram(vertShader, fragShader);
    }

    bool Shader::IsValid() const
    {
		return mProgram != 0;
    }

	void Shader::SetUniform(const std::string& name, const glm::vec3& v)
	{
		Bind();
		glUniform3fv(glGetUniformLocation(mProgram, name.c_str()), 1, glm::value_ptr(v));
		Unbind();
	}

	void Shader::SetUniform(const std::string& name, const glm::vec4& v)
	{
		Bind();
		glUniform4fv(glGetUniformLocation(mProgram, name.c_str()), 1, glm::value_ptr(v));
		Unbind();
	}

	void Shader::SetUniform(const std::string& name, const glm::mat4& v, bool transpose)
	{
		Bind();
		glUniformMatrix4fv(glGetUniformLocation(mProgram, name.c_str()), 1, transpose, glm::value_ptr(v));
		Unbind();
	}

	void Shader::SetUniform(const std::string& name, int v)
	{
		Bind();
		glUniform1i(glGetUniformLocation(mProgram, name.c_str()), v);
		Unbind();
	}

	void Shader::SetUniform(const std::string& name, float v)
	{
		Bind();
		glUniform1f(glGetUniformLocation(mProgram, name.c_str()), v);
		Unbind();
	}

    GLuint Shader::Compile(GLenum shaderType, const std::string& source) const
    {
		GLuint shaderID = glCreateShader(shaderType);
		const char* c_source = source.c_str();
		glShaderSource(shaderID, 1, &c_source, 0);
		glCompileShader(shaderID);

#ifdef _DEBUG
		GLint errorValue = 0;
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &errorValue);
		if (!errorValue)
		{
			std::string message;
			message.resize(1024);
			glGetShaderInfoLog(shaderID, message.capacity(), 0, message.data());
			std::cout << "Failed to Compile Shader " << '\n' << message << std::endl;
			throw std::runtime_error("Failed to Compile Shader");
		}
#endif // _DEBUG

		return shaderID;
    }

	GLuint Shader::CreateProgram(GLuint vertShader, GLuint fragShader) const
	{
		GLuint program = glCreateProgram();

		glAttachShader(program, vertShader);
		glAttachShader(program, fragShader);

		glLinkProgram(program);

		glDeleteShader(vertShader);
		glDeleteShader(fragShader);

#ifdef _DEBUG
		GLint errorValue = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &errorValue);
		if (!errorValue)
		{
			std::string message;
			message.resize(1024);
			glGetProgramInfoLog(program, message.capacity(), 0, message.data());
			std::cout << "Failed to Link OpenGL Program\n" << message << std::endl;
			throw std::runtime_error("Failed to Link OpenGL Program");
		}

		glValidateProgram(program);

		errorValue = 0;
		glGetProgramiv(program, GL_VALIDATE_STATUS, &errorValue);
		if (!errorValue)
		{
			std::string message;
			message.resize(1024);
			glGetProgramInfoLog(program, message.capacity(), 0, message.data());
			std::cout << "Failed to Validate OpenGL Program\n" << message << std::endl;
			throw std::runtime_error("Failed to Validate OpenGL Program");
		}
#endif // _DEBUG

		return program;
	}

	void Shader::Bind()
	{
		glGetIntegerv(GL_CURRENT_PROGRAM, &mPrevious);

		glUseProgram(mProgram);
	}

	void Shader::Unbind()
	{
		glUseProgram(mPrevious);

		mPrevious = 0;
	}
}
