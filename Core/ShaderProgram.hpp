/*****************************************************************//**
 * \file   ShaderProgram.hpp
 * \brief  Holds logic for using a shader program
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include <glew.h>
#include <glm.hpp>
#include <GLFW/glfw3.h>
#include <set>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace Gep
{
		class ShaderProgram
		{
		public:
				ShaderProgram();

				void LoadFragmentShader(const std::filesystem::path& shaderPath);
				void LoadComputeShader(const std::filesystem::path& shaderPath);
				void LoadVertexShader(const std::filesystem::path& shaderPath);
				void Compile();
				GLuint GetProgramID() const;

		private:
				GLuint LoadShader(GLenum shaderType, const std::filesystem::path& shaderPath) const;

		private:
				GLuint mProgram;
				std::set<GLuint> mShaders;
		};
}

