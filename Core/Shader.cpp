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
	Shader Shader::FromFile(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath)
	{
		std::string vertSrc = ReadShader(vertPath);
		std::string fragSrc = ReadShader(fragPath);

		Shader newShader = FromSource(vertSrc, fragSrc);

		newShader.mVertPath = vertPath;
		newShader.mFragPath = fragPath;

		return std::move(newShader);
	}

	Shader Shader::FromSource(const std::string& vertSrc, const std::string& fragSrc)
	{
		Shader newShader{};

		GLuint vertShader = Compile(GL_VERTEX_SHADER, vertSrc);
		GLuint fragShader = Compile(GL_FRAGMENT_SHADER, fragSrc);

		if (vertShader && fragShader)
			newShader.mProgram = CreateProgram(vertShader, fragShader);

		return std::move(newShader);
	}

	Shader::~Shader()
	{
		if (IsValid())
			glDeleteProgram(mProgram);
	}

	Shader::Shader(Shader&& other) noexcept
		: mProgram(other.mProgram)
	{
		other.mProgram = 0;
		mVertPath.swap(other.mVertPath);
		mFragPath.swap(other.mFragPath);
	}

	Shader& Shader::operator=(Shader&& other) noexcept
	{
		if (this != &other)
		{
			if (IsValid())
				glDeleteProgram(mProgram);

			mProgram = other.mProgram;
			other.mProgram = 0;

			mVertPath.swap(other.mVertPath);
			mFragPath.swap(other.mFragPath);
		}

		return *this;
	}

	bool Shader::IsValid() const
    {
		return mProgram != 0;
    }

	void Shader::SetUniform(const std::string& name, const glm::vec3& v)
	{
		SetUniform(glGetUniformLocation(mProgram, name.c_str()), v);
	}

	void Shader::SetUniform(const std::string& name, const glm::vec4& v)
	{
		SetUniform(glGetUniformLocation(mProgram, name.c_str()), v);
	}

	void Shader::SetUniform(const std::string& name, const glm::mat4& v, bool transpose)
	{
		SetUniform(glGetUniformLocation(mProgram, name.c_str()), v, transpose);
	}

	void Shader::SetUniform(const std::string& name, int v)
	{
		SetUniform(glGetUniformLocation(mProgram, name.c_str()), v);
	}

	void Shader::SetUniform(const std::string& name, float v)
	{
		SetUniform(glGetUniformLocation(mProgram, name.c_str()), v);
	}

	void Shader::SetUniform(const std::string& name, uint32_t v)
	{
		SetUniform(glGetUniformLocation(mProgram, name.c_str()), v);
	}

	void Shader::SetUniform(const std::string& name, uint64_t v)
	{
		SetUniform(glGetUniformLocation(mProgram, name.c_str()), v);
	}

	void Shader::SetUniform(size_t location, const glm::vec3& v)
	{
		Bind();
		glUniform3fv(location, 1, glm::value_ptr(v));
	}

	void Shader::SetUniform(size_t location, const glm::vec4& v)
	{
		Bind();
		glUniform4fv(location, 1, glm::value_ptr(v));
	}

	void Shader::SetUniform(size_t location, const glm::mat4& v, bool transpose)
	{
		Bind();
		glUniformMatrix4fv(location, 1, transpose, glm::value_ptr(v));
	}

	void Shader::SetUniform(size_t location, int v)
	{
		Bind();
		glUniform1i(location, v);
	}

	void Shader::SetUniform(size_t location, float v)
	{
		Bind();
		glUniform1f(location, v);
	}

	void Shader::SetUniform(size_t location, uint32_t v)
	{
		Bind();
		glUniform1ui(location, v);
	}

	void Shader::SetUniform(size_t location, uint64_t v)
	{
		Bind();
		glUniformHandleui64ARB(location, v);
	}

    GLuint Shader::Compile(GLenum shaderType, const std::string& source)
    {
		GLuint shaderID = glCreateShader(shaderType);
		const char* c_source = source.c_str();
		glShaderSource(shaderID, 1, &c_source, 0);
		glCompileShader(shaderID);

		GLint errorValue = 0;
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &errorValue);
		if (!errorValue)
		{
			std::string message;
			message.resize(1024);
			glGetShaderInfoLog(shaderID, message.capacity(), 0, message.data());

			Gep::Log::Error("Failed to Compile Shader\n", message);
			Gep::Log::Error("\n", source);

			return 0; // failed to compile
		}

		return shaderID;
    }

	GLuint Shader::CreateProgram(GLuint vertShader, GLuint fragShader)
	{
		GLuint program = glCreateProgram();

		glAttachShader(program, vertShader);
		glAttachShader(program, fragShader);

		glLinkProgram(program);

		glDeleteShader(vertShader);
		glDeleteShader(fragShader);

		GLint errorValue = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &errorValue);
		if (!errorValue)
		{
			std::string message;
			message.resize(1024);
			glGetProgramInfoLog(program, message.capacity(), 0, message.data());

			Gep::Log::Error("Failed to Link OpenGL Program\n", message);

			return 0; // failed to link
		}

		glValidateProgram(program);

		errorValue = 0;
		glGetProgramiv(program, GL_VALIDATE_STATUS, &errorValue);
		if (!errorValue)
		{
			std::string message;
			message.resize(1024);
			glGetProgramInfoLog(program, message.capacity(), 0, message.data());

			Gep::Log::Error("Failed to Validate OpenGL Program\n", message);

			return 0; // failed to Validate
		}

		return program;
	}

	std::string Shader::ReadShader(const std::filesystem::path& path)
	{
		std::string shaderSource = Gep::ReadFile(path);
		
		const std::string searchString = "#include";
		size_t includeIndex = shaderSource.find(searchString);
		size_t linestartIndex = shaderSource.rfind('\n', includeIndex) + 1; // 1 after the new line is the current line start
		size_t lineEndIndex = shaderSource.find('\n', linestartIndex);
		while (includeIndex != std::string::npos)
		{
			size_t quoteIndex1 = shaderSource.find('\"', includeIndex + searchString.size());

			if (quoteIndex1 == std::string::npos)
				throw std::runtime_error("Invalid syntax around [#include] in shader");

			size_t quoteIndex2 = shaderSource.find('\"', quoteIndex1 + 1);
			if (quoteIndex2 == std::string::npos)
				throw std::runtime_error("Invalid syntax around [#include] in shader");

			std::string includeFileName = shaderSource.substr(quoteIndex1 + 1, quoteIndex2 - quoteIndex1 - 1); // get the name of the file included

			std::string includedSource = Gep::ReadFile(path.parent_path() / includeFileName); // read in the included files source

			shaderSource.erase(linestartIndex, lineEndIndex - linestartIndex); // remove the include line

			shaderSource.insert(linestartIndex, includedSource); // inserts the included source into where the include statement was

			includeIndex = shaderSource.find(searchString, linestartIndex); // move to the next include if there is once
		}

		return shaderSource;
	}

	void Shader::Reload()
	{
		Shader newShader = FromFile(mVertPath, mFragPath);
		 
		if (newShader.IsValid())
		{
			*this = std::move(newShader);
		}
	}

	void Shader::Bind()
	{
		glUseProgram(mProgram);
	}

	void Shader::Unbind()
	{
		glUseProgram(0);
	}
}
