/*****************************************************************//**
 * \file   ShaderProgram.hpp
 * \brief  Holds logic for using a shader program
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include <Core.hpp>
#include <glew.h>

#include <glm.hpp>
#include <GLFW/glfw3.h>

namespace Gep
{
	class ShaderProgram
	{
	public:
		ShaderProgram()
		: mProgram(glCreateProgram())
		, mShaders()
		{}

		void LoadFragmentShader(const std::filesystem::path& shaderPath)
		{
			GLuint shader = LoadShader(GL_FRAGMENT_SHADER, shaderPath);
			mShaders.insert(shader);
		}

		void LoadComputeShader(const std::filesystem::path& shaderPath)
		{
			GLuint shader = LoadShader(GL_COMPUTE_SHADER, shaderPath);
			mShaders.insert(shader);
		}

		void LoadVertexShader(const std::filesystem::path& shaderPath)
		{
			GLuint shader = LoadShader(GL_VERTEX_SHADER, shaderPath);
			mShaders.insert(shader);
		}

		void Compile()
		{
			// attach all of the loaded shaders
			for (GLuint shader : mShaders)
			{
				glAttachShader(mProgram, shader);
			}

			// links the program
			glLinkProgram(mProgram);

			// checks for success only in debug mode
#ifdef _DEBUG
			GLint errorValue = 0;
			glGetProgramiv(mProgram, GL_LINK_STATUS, &errorValue);
			if (!errorValue) // 0 means success
			{
				// creates a message buffer
				std::string message;
				message.resize(1024);

				// puts the gl info log into the buffer and prints it
				glGetProgramInfoLog(mProgram, message.capacity(), 0, message.data());
				std::cout << "Failed to Link OpenGL Program\n" << message << std::endl;
				throw std::runtime_error("Failed to Link OpenGL Program");
			}

			// validates the program
			glValidateProgram(mProgram);

			// checks for success
			errorValue = 0;
			glGetProgramiv(mProgram, GL_VALIDATE_STATUS, &errorValue);
			if (!errorValue) // 0 means success
			{
				// creates a message buffer
				std::string message;
				message.resize(1024);

				// puts the gl info log into the buffer and prints it
				glGetProgramInfoLog(mProgram, message.capacity(), 0, message.data());
				std::cout << "Failed to Validate OpenGL Program\n" << message << std::endl;
				throw std::runtime_error("Failed to Validate OpenGL Program");
			}
#endif // _DEBUG
			// cleanup shaders
			for (GLuint shader : mShaders)
			{
				glDeleteShader(shader);
			}
			mShaders.clear();

			// turn on depth buffer
			glEnable(GL_DEPTH_TEST);
		}

		GLuint GetProgramID() const
		{
			return mProgram;
		}

	private:
		// loads a shader of the given type at the given path, GL_FRAGMENT_SHADER GL_VERTEX_SHADER, GL_COMPUTE_SHADER
		GLuint LoadShader(GLenum shaderType, const std::filesystem::path& shaderPath) const
		{
			std::string source;

			// opens a file
			std::ifstream inFile(shaderPath);
			assert(!(!inFile.is_open()) && "Failed to open shader file");

			//reads in all of the data from the file
			source.assign((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
			inFile.close();

			// create and compile the shader
			GLuint shaderID = glCreateShader(shaderType);
			const char* c_source = source.c_str();
			glShaderSource(shaderID, 1, &c_source, 0);
			glCompileShader(shaderID);

			// check for success
#ifdef _DEBUG
			GLint errorValue = 0;
			glGetShaderiv(shaderID, GL_COMPILE_STATUS, &errorValue);
			if (!errorValue) // 0 means success
			{
				// creates a message buffer
				std::string message;
				message.resize(1024);

				// puts the gl info log into the buffer and prints it
				glGetShaderInfoLog(shaderID, message.capacity(), 0, message.data());
				std::cout << "Failed to Compile Shader " << shaderPath.string() << '\n' << message << std::endl;
				throw std::runtime_error("Failed to Compile Shader");
			}
#endif // _DEBUG
			return shaderID;
		}

		// the program being used
		GLuint mProgram;

		// the shaders used by the program
		std::set<GLuint> mShaders;
	};
}
