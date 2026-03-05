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
	static std::unordered_map<std::filesystem::path, std::string> mShaderCache;

	static GLuint Compile(GLenum shaderType, const std::string& source, const std::string& origin = "<embedded>")
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
			Gep::Log::Error("Origin: ", origin);

			return 0; // failed to compile
		}

		return shaderID;
	}

	static std::string ReadShader(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
		{
			Gep::Log::Warning("ReadShader(), Attempting to read from a path that doesnt exist ", path);
			return "";
		}

		const std::string searchString = "#include";
		std::string shaderSource = Gep::ReadFile(path);

		size_t includeIndex = shaderSource.find(searchString);

		// if it successfully finds an include evaluate it
		while (includeIndex != std::string::npos)
		{
			// locate the position of both start and end quotes
			size_t quoteIndex1 = shaderSource.find('\"', includeIndex + searchString.size());
			size_t quoteIndex2 = shaderSource.find('\"', quoteIndex1 + 1);

			// get the name of the file included
			const std::string includeFileName = shaderSource.substr(quoteIndex1 + 1, quoteIndex2 - quoteIndex1 - 1);

			// remove the [#include "..."]
			shaderSource.erase(includeIndex, quoteIndex2 - includeIndex + 1);

			const std::filesystem::path includePath = path.parent_path() / includeFileName;
			size_t offset = includeIndex; // minor optimization skips already checked areas

			// if the include file has already been loaded reuse it
			if (mShaderCache.contains(includePath))
			{
				const std::string& includeSource = mShaderCache.at(includePath);
				shaderSource.insert(includeIndex, includeSource);
				offset += includeSource.size();
			}
			// load the include file from disk and add it to the cache
			else if (std::filesystem::exists(includePath))
			{
				const auto& [it, inserted] = mShaderCache.try_emplace(includePath, ReadShader(includePath));
				const auto& [_, includeSource] = *it;

				shaderSource.insert(includeIndex, includeSource);
				offset += includeSource.size();
			}
			else
			{
				Gep::Log::Warning("Include doesn't exist in [", path, "]: [#include \"", includeFileName, "\"]");
			}

			includeIndex = shaderSource.find(searchString, offset);
		}

		return shaderSource;
	}

	Shader Shader::FromFile(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath, const std::filesystem::path& geomPath)
	{
		Shader newShader{};

        if (!std::filesystem::exists(vertPath))
		{
			Gep::Log::Error("Vertex Shader file does not exist: ", vertPath);
			return newShader;
		}

		if (!std::filesystem::exists(fragPath))
		{
			Gep::Log::Error("Fragment Shader file does not exist: ", fragPath);
			return newShader;
		}

		if (!geomPath.empty() && !std::filesystem::exists(geomPath))
		{
			Gep::Log::Error("Geometry Shader file does not exist: ", geomPath);
			return newShader;
        }

		std::string vertSrc = ReadShader(vertPath);
		std::string fragSrc = ReadShader(fragPath);
		GLuint vertShader = Compile(GL_VERTEX_SHADER, vertSrc, vertPath.string());
		GLuint fragShader = Compile(GL_FRAGMENT_SHADER, fragSrc, fragPath.string());

		GLuint geomShader = NULL;
		if (!geomPath.empty())
		{
			std::string geomSrc = ReadShader(geomPath);
            geomShader = Compile(GL_GEOMETRY_SHADER, geomSrc, geomPath.string());
		}

		if (vertShader && fragShader)
		{
            std::string origin = "(" + vertPath.string() + " + " + fragPath.string() + ")";
			newShader.mProgram = CreateProgram(vertShader, fragShader, geomShader, origin);
		}

		newShader.mVertPath = vertPath;
		newShader.mFragPath = fragPath;
        newShader.mGeomPath = geomPath;

		return newShader;
	}

	Shader Shader::FromSource(const std::string& vertSrc, const std::string& fragSrc, const std::string& geomSrc)
	{
		Shader newShader{};

		GLuint vertShader = Compile(GL_VERTEX_SHADER, vertSrc);
		GLuint fragShader = Compile(GL_FRAGMENT_SHADER, fragSrc);
		GLuint geomShader = NULL;
        if (!geomSrc.empty())
            geomShader = Compile(GL_GEOMETRY_SHADER, geomSrc);

		if (vertShader && fragShader)
			newShader.mProgram = CreateProgram(vertShader, fragShader, geomShader);

		return newShader;
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
        mGeomPath.swap(other.mGeomPath);
	}

	Shader& Shader::operator=(Shader&& other) noexcept
	{
		if (this != &other)
		{
			if (IsValid())
				glDeleteProgram(mProgram);

			mProgram = other.mProgram;
			mVertPath.swap(other.mVertPath);
			mFragPath.swap(other.mFragPath);
            mGeomPath.swap(other.mGeomPath);

			other.mProgram = 0;
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

	GLuint Shader::CreateProgram(GLuint vertShader, GLuint fragShader, GLuint geomShader, const std::string& origin)
	{
		GLuint program = glCreateProgram();

		glAttachShader(program, vertShader);
		glAttachShader(program, fragShader);
        if (geomShader)
			glAttachShader(program, geomShader);

		glLinkProgram(program);

		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
        if (geomShader)
			glDeleteShader(geomShader);

		GLint errorValue = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &errorValue);
		if (!errorValue)
		{
			std::string message;
			message.resize(1024);
			glGetProgramInfoLog(program, message.size(), 0, message.data());

			Gep::Log::Error("Failed to Link OpenGL Program\n", message);
            Gep::Log::Error("Origin: ", origin);

			return 0; // failed to link
		}

		glValidateProgram(program);

		errorValue = 0;
		glGetProgramiv(program, GL_VALIDATE_STATUS, &errorValue);
		if (!errorValue)
		{
			std::string message;
			message.resize(1024);
			glGetProgramInfoLog(program, message.size(), 0, message.data());

			Gep::Log::Error("Failed to Validate OpenGL Program\n", message);
            Gep::Log::Error("Origin: ", origin);

			return 0; // failed to Validate
		}

		return program;
	}

	void Shader::Reload()
	{
		mShaderCache.clear();

		Shader newShader = FromFile(mVertPath, mFragPath, mGeomPath);
		 
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

	ComputeShader ComputeShader::FromFile(const std::filesystem::path& compPath)
	{
		ComputeShader newShader{};

		if (!std::filesystem::exists(compPath))
		{
			Gep::Log::Error("Compute Shader file does not exist: ", compPath);
			return newShader;
		}

		std::string compSrc = ReadShader(compPath);

		GLuint compShader = Compile(GL_COMPUTE_SHADER, compSrc, compPath.string());

		if (compShader)
		{
			std::string origin = "(" + compPath.string() + ")";
			newShader.mProgram = CreateProgram(compShader, origin);

			glGetProgramiv(newShader.mProgram, GL_COMPUTE_WORK_GROUP_SIZE, &newShader.mWorkGroupSize[0]);
		}

		newShader.mCompPath = compPath;

		return newShader;
	}

	ComputeShader ComputeShader::FromSource(const std::string& compSrc)
	{
		ComputeShader newShader{};

		GLuint compShader = Compile(GL_COMPUTE_SHADER, compSrc);

		if (compShader)
		{
			newShader.mProgram = CreateProgram(compShader);

			glGetProgramiv(newShader.mProgram, GL_COMPUTE_WORK_GROUP_SIZE, &newShader.mWorkGroupSize[0]);
		}

		return newShader;
	}

	ComputeShader::~ComputeShader()
	{
		if (IsValid())
			glDeleteProgram(mProgram);
	}

	ComputeShader::ComputeShader(ComputeShader&& other) noexcept
		: mProgram(other.mProgram)
		, mWorkGroupSize(other.mWorkGroupSize)
	{
		other.mProgram = 0;
		mCompPath.swap(other.mCompPath);
	}

	ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept
	{
		if (this != &other)
		{
			if (IsValid())
				glDeleteProgram(mProgram);

			mWorkGroupSize = other.mWorkGroupSize;
			mProgram = other.mProgram;
			mCompPath.swap(other.mCompPath);

			other.mWorkGroupSize = {};
			other.mProgram = 0;
		}

		return *this;
	}

	void ComputeShader::Dispatch(glm::uvec3 size)
	{
		if (size.x % mWorkGroupSize.x != 0
		 || size.y % mWorkGroupSize.y != 0
		 || size.z % mWorkGroupSize.z != 0)
		{
			Gep::Log::Error("Dispatch(): Cannot evenly distribute compute task.");
			return;
		}

		glDispatchCompute(size.x / mWorkGroupSize.x, size.y / mWorkGroupSize.y, size.z / mWorkGroupSize.z);
	}

	bool ComputeShader::IsValid() const
	{
		return mProgram != 0;
	}

	void ComputeShader::Bind()
	{
		glUseProgram(mProgram);
	}

	void ComputeShader::Unbind()
	{
		glUseProgram(0);
	}

	GLuint ComputeShader::CreateProgram(GLuint computeShader, const std::string& origin)
	{
		GLuint program = glCreateProgram();

		glAttachShader(program, computeShader);
		glLinkProgram(program);
		glDeleteShader(computeShader);

		GLint errorValue = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &errorValue);
		if (!errorValue)
		{
			std::string message;
			message.resize(1024);
			glGetProgramInfoLog(program, message.size(), 0, message.data());

			Gep::Log::Error("Failed to Link OpenGL Program\n", message);
			Gep::Log::Error("Origin: ", origin);

			return 0; // failed to link
		}

		glValidateProgram(program);

		errorValue = 0;
		glGetProgramiv(program, GL_VALIDATE_STATUS, &errorValue);
		if (!errorValue)
		{
			std::string message;
			message.resize(1024);
			glGetProgramInfoLog(program, message.size(), 0, message.data());

			Gep::Log::Error("Failed to Validate OpenGL Program\n", message);
			Gep::Log::Error("Origin: ", origin);

			return 0; // failed to Validate
		}

		return program;
	}

}
