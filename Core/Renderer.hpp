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
//#include <GLFW/glfw3.h>

#include <Mesh.hpp>
#include <Camera.hpp>
#include <CompactArray.hpp>

namespace Gep
{
	class IRenderer
	{
	public:
		IRenderer()
			: mProgram(glCreateProgram())
			, mShaders()
			, mMeshDatas()
		{
		}

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
			GLuint shader = LoadShader(GL_COMPUTE_SHADER, shaderPath);
			mShaders.insert(shader);
		}

		virtual void LoadVertexShader(const std::filesystem::path& shaderPath) final
		{
			GLuint shader = LoadShader(GL_VERTEX_SHADER, shaderPath);
			mShaders.insert(shader);
		}

		virtual void Compile() final
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

		virtual std::uint64_t LoadMesh(const NormalMesh& mesh)
		{
			// add the mesh into the internal container
			const std::uint64_t meshID = mMeshDatas.emplace();
			MeshData& meshData = mMeshDatas.at(meshID);

			// generate the buffers and bind them to the vao
			meshData.GenVertexBuffer(mesh);
			meshData.GenNormalBuffer(mesh);
			meshData.GenFaceBuffer(mesh);
			meshData.BindBuffers();
			meshData.mEdgeCount = mesh.mEdges.size();
			
			return meshID;
		}

		virtual void UnloadMesh(std::uint64_t meshID)
		{
			MeshData& meshData = mMeshDatas.at(meshID);

			meshData.DeleteBuffers();

			mMeshDatas.erase(meshID);
		}

		// wether or not to cull backfaces
		virtual void BackfaceCull(bool enabled = true) final
		{
			if (enabled)
			{
				glEnable(GL_CULL_FACE);
			}
			else
			{
				glDisable(GL_CULL_FACE);
			}
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
			const glm::mat4 pers = camera.GetPerspective();
			const glm::mat4 view = camera.GetView();
			const glm::vec4 eye  = camera.GetEyePosition();

			glUseProgram(mProgram);
			// location -------.
			//                 |
			// amount ---------+--.
			//                 |  |
			// transpose ------+--+--.
			//                 |  |  |
			// data -----------+--+--+------.
			//                 |  |  |      |
			glUniformMatrix4fv(0, 1, false, &pers[0][0]);
			glUniformMatrix4fv(1, 1, false, &view[0][0]);
			glUniform4fv      (4, 1, &eye[0]);

			glUseProgram(0); // deselect program
		}

		// sets the modeling transformation
		virtual void SetModel(const glm::mat4& modelingMatrix) final
		{
			// double cast to truncate uneeded data
			glm::mat4 normal = glm::mat4(glm::mat3(affine_inverse(modelingMatrix)));

			glUseProgram(mProgram);

			glUniformMatrix4fv(2, 1, false, &modelingMatrix[0][0]);
			glUniformMatrix4fv(3, 1, true,  &normal[0][0]);

			glUseProgram(0); // deselect program
		}

		virtual void SetMaterial(const glm::vec3& diffuseCoeff, const glm::vec3& specularCoeff, float specularExponent) final
		{
			glUseProgram(mProgram);

			glUniform3fv(5, 1, &diffuseCoeff[0]);
			glUniform3fv(6, 1, &specularCoeff[0]);
			glUniform1fv(7, 1, &specularExponent);

			glUseProgram(0); // deselect program
		}

		virtual void CreateLight(const std::uint8_t lightID, const glm::vec3& position, const glm::vec3& color)
		{
			glUseProgram(mProgram);

			int True = 1; // lvalue for grandpa openGL

			const std::uint8_t lightLimit = 8;

			glUniform4fv(9 + lightID, 1, &position[0]);
			glUniform3fv(17 + lightID, 1, &color[0]);
			glUniform1iv(25 + lightID, 1, &True);

			glUseProgram(0);
		}

		// sets the ambient light for the current renderer
		virtual void SetAmbientLight(const glm::vec3& color)
		{
			glUseProgram(mProgram);

			glUniform3fv(8, 1, &color[0]);

			glUseProgram(0); // deselect program
		}

		// the mesh in the mesh id to draw
		virtual void DrawMesh(std::uint64_t meshID) const
		{
			// checks if the mesh id is valid
			const MeshData& md = mMeshDatas.at(meshID); 
			constexpr std::uint64_t faceSize = sizeof(Mesh::Face) / sizeof(GLuint);
			constexpr std::uint64_t edgeSize = sizeof(Mesh::Edge) / sizeof(GLuint);

			glUseProgram(mProgram); // select program 
			glBindVertexArray(md.mVertexArrayObject); // select vao

			// draws all of the triangles

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElements(GL_TRIANGLES, faceSize * md.mFaceCount, GL_UNSIGNED_INT, 0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			glBindVertexArray(0); // deselect vao
			glUseProgram(0); // deselect program

			
		}


	private:

		// loads a shader of the given type at the given path, GL_FRAGMENT_SHADER GL_VERTEX_SHADER, GL_COMPUTE_SHADER
		GLuint LoadShader(GLenum shaderType, const std::filesystem::path& shaderPath)
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

	private:
		struct MeshData
		{
		public:
			MeshData()
				: mVertexArrayObject(num_max<GLuint>())
				, mVertexBuffer(num_max<GLuint>())
				, mNormalBuffer(num_max<GLuint>())
				, mFaceBuffer(num_max<GLuint>())
				, mFaceCount(num_max<size_t>())
				, mEdgeCount(num_max<size_t>())
			{

			}

			~MeshData()
			{
			}

			void GenVertexBuffer(const Mesh& mesh)
			{
				glGenBuffers(1, &mVertexBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
				glBufferData(GL_ARRAY_BUFFER, sizeof(Mesh::Vertex) * mesh.mVertices.size(), mesh.mVertices.data(), GL_STATIC_DRAW);
			}
			
			void GenNormalBuffer(const NormalMesh& mesh)
			{
				glGenBuffers(1, &mNormalBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffer);
				glBufferData(GL_ARRAY_BUFFER, sizeof(NormalMesh::Normal) * mesh.mNormals.size(), mesh.mNormals.data(), GL_STATIC_DRAW);
			}
			
			void GenFaceBuffer(const Mesh& mesh)
			{
				glGenBuffers(1, &mFaceBuffer);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mFaceBuffer);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Mesh::Face) * mesh.mFaces.size(), mesh.mFaces.data(), GL_STATIC_DRAW);

				mFaceCount = mesh.mFaces.size();
			}

			// called after all other buffers are generated to finalize vao
			void BindBuffers()
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

			void DeleteBuffers()
			{
				glDeleteBuffers(1, &mFaceBuffer);
				glDeleteBuffers(1, &mNormalBuffer);
				glDeleteBuffers(1, &mVertexBuffer);

				glDeleteVertexArrays(1, &mVertexArrayObject);

#ifdef _DEBUG
				mVertexArrayObject = num_max<GLuint>();
				mVertexBuffer = num_max<GLuint>();
				mNormalBuffer = num_max<GLuint>();
				mFaceBuffer = num_max<GLuint>();
				mFaceCount = num_max<GLuint>();
#endif // _DEBUG

			}
		public:
			GLuint mVertexArrayObject;
			GLuint mVertexBuffer;
			GLuint mNormalBuffer;
			GLuint mFaceBuffer;
			size_t mFaceCount;
			size_t mEdgeCount;
		};

	private:
		// the program being useds
		GLuint mProgram;

		// the shaders used by the program
		std::set<GLuint> mShaders;

		compact_array<MeshData> mMeshDatas;
	};
}
