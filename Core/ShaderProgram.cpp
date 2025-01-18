/*****************************************************************//**
 * \file   ShaderProgram.cpp
 * \brief  Holds logic for using a shader program
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "ShaderProgram.hpp"

namespace Gep
{
		ShaderProgram::ShaderProgram()
				: mProgram(glCreateProgram())
				, mShaders()
		{
		}

		void ShaderProgram::LoadFragmentShader(const std::filesystem::path& shaderPath)
		{
				GLuint shader = LoadShader(GL_FRAGMENT_SHADER, shaderPath);
				mShaders.insert(shader);
		}

		void ShaderProgram::LoadComputeShader(const std::filesystem::path& shaderPath)
		{
				GLuint shader = LoadShader(GL_COMPUTE_SHADER, shaderPath);
				mShaders.insert(shader);
		}

		void ShaderProgram::LoadVertexShader(const std::filesystem::path& shaderPath)
		{
				GLuint shader = LoadShader(GL_VERTEX_SHADER, shaderPath);
				mShaders.insert(shader);
		}

		void ShaderProgram::Compile()
		{
				for (GLuint shader : mShaders)
				{
						glAttachShader(mProgram, shader);
				}

				glLinkProgram(mProgram);

#ifdef _DEBUG
				GLint errorValue = 0;
				glGetProgramiv(mProgram, GL_LINK_STATUS, &errorValue);
				if (!errorValue)
				{
						std::string message;
						message.resize(1024);
						glGetProgramInfoLog(mProgram, message.capacity(), 0, message.data());
						std::cout << "Failed to Link OpenGL Program\n" << message << std::endl;
						throw std::runtime_error("Failed to Link OpenGL Program");
				}

				glValidateProgram(mProgram);

				errorValue = 0;
				glGetProgramiv(mProgram, GL_VALIDATE_STATUS, &errorValue);
				if (!errorValue)
				{
						std::string message;
						message.resize(1024);
						glGetProgramInfoLog(mProgram, message.capacity(), 0, message.data());
						std::cout << "Failed to Validate OpenGL Program\n" << message << std::endl;
						throw std::runtime_error("Failed to Validate OpenGL Program");
				}
#endif // _DEBUG

				for (GLuint shader : mShaders)
				{
						glDeleteShader(shader);
				}
				mShaders.clear();

				glEnable(GL_DEPTH_TEST);
		}

		GLuint ShaderProgram::GetProgramID() const
		{
				return mProgram;
		}

		GLuint ShaderProgram::LoadShader(GLenum shaderType, const std::filesystem::path& shaderPath) const
		{
				std::string source;

				std::ifstream inFile(shaderPath);
				assert(!(!inFile.is_open()) && "Failed to open shader file");

				source.assign((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
				inFile.close();

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
						std::cout << "Failed to Compile Shader " << shaderPath.string() << '\n' << message << std::endl;
						throw std::runtime_error("Failed to Compile Shader");
				}
#endif // _DEBUG
				return shaderID;
		}
}

