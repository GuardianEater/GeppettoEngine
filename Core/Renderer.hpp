/*****************************************************************//**
 * \file   Renderer.hpp
 * \brief  Base interface for the type of rendering being performed
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include <glew.h>

#include <glm.hpp>
#include <GLFW/glfw3.h>

#include <Mesh.hpp>
#include <Camera.hpp>

namespace Gep
{
	class IRenderer
	{
	public:
		IRenderer()
			: mProgram(glCreateProgram())
			, mShaders()
			, mMeshDatas()
			, mNextMeshID(0)
		{}

		~IRenderer()
		{
			mMeshDatas.clear();
		}

		virtual void LoadFragmentShader(const std::filesystem::path& shaderPath) final
		{
			GLuint shader = LoadShader(GL_FRAGMENT_SHADER, shaderPath);
			mShaders.insert(shader);
		}

		virtual void LoadComputeShader(const std::filesystem::path& shaderPath) final
		{
			GLuint shader = LoadShader(GL_FRAGMENT_SHADER, shaderPath);
			mShaders.insert(shader);
		}

		virtual void LoadVertexShader(const std::filesystem::path& shaderPath) final
		{
			GLuint shader = LoadShader(GL_FRAGMENT_SHADER, shaderPath);
			mShaders.insert(shader);
		}

		virtual void Compile() final
		{
			// attach all of the loaded shaders
			for (GLuint shader : mShaders)
			{
				glAttachShader(shader, mProgram);
			}

			// links the program
			glLinkProgram(mProgram);

			// checks for success
			GLint errorValue = 0;
			glGetProgramiv(mProgram, GL_LINK_STATUS, &errorValue);
			if (errorValue) // 0 means success
			{
				// creates a message buffer
				std::string message;
				message.reserve(1024);

				// puts the gl info log into the buffer and prints it
				glGetProgramInfoLog(mProgram, message.capacity(), 0, message.data());
				std::cout << "Failed to Link OpenGL Program\n" << message << std::endl;
				throw std::runtime_error("Failed to Link OpenGL Program");
			}

			// cleanup shaders
			for (GLuint shader : mShaders)
			{
				glDeleteShader(shader);
			}
			mShaders.clear();

			// turn on depth buffer
			glEnable(GL_DEPTH_TEST);
		}

		// will load a mesh with the given name and return the meshes id
		virtual std::uint64_t LoadMesh(const std::filesystem::path& meshFileName) final
		{
			// later potentially externally load the meshes so different renderers dont have to reload meshes
			Mesh mesh;
			mesh.Read(meshFileName);
			
			++mNextMeshID;

			// contructs a meshdata object
			mMeshDatas.emplace(mNextMeshID, mesh);

			return mNextMeshID;
		}

		virtual void UnloadMesh(std::uint64_t meshID)
		{
			mMeshDatas.erase(meshID);
		}

		// wether or not to cull backfaces
		virtual void BackfaceCull(bool enabled = true) final
		{
			if (enabled) glEnable(GL_CULL_FACE);
			else glDisable(GL_CULL_FACE);
		}

		// clears the screen with a given color
		virtual void Clear(const glm::vec3& color = {0, 0, 0}) final
		{
			glClearColor(color.r, color.g, color.b, 1);
			glClearDepth(1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		// the camera to be used for rendering
		virtual void SetCamera(const Camera& camera) final
		{
			glUseProgram(mProgram);
			// location -------.
			//                 |
			// amount ---------+--.
			//                 |  |
			// transpose ------+--+--.
			//                 |  |  |
			// data -----------+--+--+------.
			//                 |  |  |      |
			glUniformMatrix4fv(0, 1, false, &camera.GetPerspective()[0][0]);
			glUniformMatrix4fv(1, 1, false, &camera.GetView()[0][0]);
			glUniform4fv      (4, 1, &camera.GetEyePosition()[0]);
		}

		// sets the modeling transformation
		virtual void SetModel(const glm::mat4& modelingMatrix) final
		{
			glm::mat4 normal = glm::mat4(glm::mat3(glm::inverse(modelingMatrix)));

			glUseProgram(mProgram);

			glUniformMatrix4fv(2, 1, false, &modelingMatrix[0][0]);
			glUniformMatrix4fv(3, 1, true,  &normal[0][0]);
		}

		virtual void SetMaterial(const glm::vec3& diffuseCoeff, const glm::vec3& specularCoeff, float specularExponent) final
		{
			glUseProgram(mProgram);

			glUniform3fv(5, 1, &diffuseCoeff[0]);
			glUniform3fv(6, 1, &specularCoeff[0]);
			glUniform1fv(7, 1, &specularExponent);
		}

		virtual void CreateLight(std::uint8_t lightID, glm::vec3 position, glm::vec3 color)
		{

		}

		// sets the ambient light for the current renderer
		virtual void SetAmbientLight(const glm::vec3& color)
		{
			glUseProgram(0);

			glUniform3fv(8, 1, &color[0]);
		}

		// the mesh in the mesh id to draw
		virtual void DrawMesh(std::uint64_t meshID) const
		{
			// checks if the mesh id is valid
			if (!mMeshDatas.contains(meshID)) return;
			const MeshData& md = mMeshDatas.at(meshID); 

			glUseProgram(mProgram);
			
			glBindVertexArray(md.mVertexArrayObject); // select vao
			constexpr std::uint8_t faceSize = sizeof(Mesh::Face) / sizeof(GLuint);
			glDrawElements(GL_TRIANGLES, faceSize * md.mFaceCount, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0); // deselect vao
		}


	private:

		// loads a shader of the given type at the given path, GL_FRAGMENT_SHADER GL_VERTEX_SHADER, GL_COMPUTE_SHADER
		GLuint LoadShader(GLenum shaderType, const std::filesystem::path& shaderPath)
		{
			std::string source;

			// opens a file
			std::ifstream inFile(shaderPath);
			if (!inFile.is_open()) return false;

			//reads in all of the data from the file
			source.assign((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
			inFile.close();

			// create and compile the shader
			GLuint shaderID = glCreateShader(shaderType);
			const char* c_source = source.c_str();
			glShaderSource(shaderID, 1, &c_source, 0);
			glCompileShader(shaderID);

			// check for success
			GLint errorValue;
			glGetShaderiv(shaderID, GL_COMPILE_STATUS, &errorValue);
			if (!errorValue) // 0 means success
			{
				// creates a message buffer
				std::string message;
				message.reserve(1024);

				// puts the gl info log into the buffer and prints it
				glGetShaderInfoLog(shaderID, message.capacity(), 0, message.data());
				std::cout << "Failed to Compile Shader " << shaderPath.string() << '\n' << message << std::endl;
				throw std::runtime_error("Failed to Compile Shader");
			}

			return shaderID;
		}

	private:
		struct MeshData
		{
			MeshData(const Mesh& mesh)
				: mVertexArrayObject()
				, mVertexBuffer(GenVertexBuffer(mesh))
				, mNormalBuffer(GenNormalBuffer(mesh))
				, mFaceBuffer(GenFaceBuffer(mesh))
				, mFaceCount(mesh.mFaces.size())
			{
				// vao
				glGenVertexArrays(1, &mVertexArrayObject);
				glBindVertexArray(mVertexArrayObject);

				// vertex
				glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
				glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, 0); // four floats becuase that is glm::vec4
				glEnableVertexAttribArray(0);

				// normal
				glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffer);
				glVertexAttribPointer(1, 4, GL_FLOAT, false, 0, 0); // four floats becuase that is glm::vec4
				glEnableVertexAttribArray(1);

				// face
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mFaceBuffer);

				// unbind vao
				glBindVertexArray(0);
			}

			~MeshData()
			{
				glDeleteBuffers(1, &mFaceBuffer);
				glDeleteBuffers(1, &mNormalBuffer);
				glDeleteBuffers(1, &mVertexBuffer);
				
				glDeleteVertexArrays(1, &mVertexArrayObject);
			}

			GLuint mVertexArrayObject;

			GLuint mVertexBuffer;
			GLuint mNormalBuffer;
			GLuint mFaceBuffer;

			size_t mFaceCount;

		private:
			GLuint GenVertexBuffer(const Mesh& mesh) const
			{
				GLuint buffer;

				glGenBuffers(1, &buffer);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glm::vec4) * mesh.mVertices.size(), mesh.mVertices.data(), GL_STATIC_DRAW);

				return buffer;
			}
			
			GLuint GenNormalBuffer(const Mesh& mesh) const
			{
				GLuint buffer;

				glGenBuffers(1, &buffer);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glm::vec4) * mesh.mNormals.size(), mesh.mNormals.data(), GL_STATIC_DRAW);

				return buffer;
			}
			
			GLuint GenFaceBuffer(const Mesh& mesh) const
			{
				GLuint buffer;

				glGenBuffers(1, &buffer);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Mesh::Face) * mesh.mFaces.size(), mesh.mFaces.data(), GL_STATIC_DRAW);

				return buffer;
			}
		};

	private:
		// the program being useds
		GLuint mProgram;

		// the shaders used by the program
		std::set<GLuint> mShaders;

		// info about all current meshes
		std::map<std::uint64_t, MeshData> mMeshDatas;

		// the ids that are given to the client
		std::uint64_t mNextMeshID;
	};
}
