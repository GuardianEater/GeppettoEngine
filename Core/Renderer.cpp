/*****************************************************************//**
 * \file   Renderer.cpp
 * \brief  Base interface for the type of rendering being performed
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "Renderer.hpp"

namespace Gep
{
		IRenderer::IRenderer()
				: mProgram()
				, mMeshDatas()
		{
		}

		IRenderer::~IRenderer()
		{
				mMeshDatas.clear();
		}

		void IRenderer::LoadFragmentShader(const std::filesystem::path& shaderPath)
		{
				mProgram.LoadFragmentShader(shaderPath);
		}

		void IRenderer::LoadVertexShader(const std::filesystem::path& shaderPath)
		{
				mProgram.LoadVertexShader(shaderPath);
		}

		void IRenderer::Compile()
		{
				mProgram.Compile();
		}

		std::uint64_t IRenderer::LoadMesh(const NormalMesh& mesh)
		{
				const std::uint64_t meshID = mMeshDatas.emplace();
				MeshData& meshData = mMeshDatas.at(meshID);

				meshData.GenVertexBuffer(mesh);
				meshData.GenNormalBuffer(mesh);
				meshData.GenFaceBuffer(mesh);
				meshData.BindBuffers();
				meshData.mEdgeCount = mesh.mEdges.size();

				return meshID;
		}

		void IRenderer::UnloadMesh(std::uint64_t meshID)
		{
				MeshData& meshData = mMeshDatas.at(meshID);
				meshData.DeleteBuffers();
				mMeshDatas.erase(meshID);
		}

		void IRenderer::BackfaceCull(bool enabled)
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

		void IRenderer::Clear(const glm::vec3& color)
		{
				glClearColor(color.r, color.g, color.b, 1);
				glClearDepth(1);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		void IRenderer::SetCamera(const Camera& camera)
		{
				const glm::mat4 pers = camera.GetPerspective();
				const glm::mat4 view = camera.GetView();
				const glm::vec4 eye = camera.GetEyePosition();

				glUseProgram(mProgram.GetProgramID());
				glUniformMatrix4fv(0, 1, false, &pers[0][0]);
				glUniformMatrix4fv(1, 1, false, &view[0][0]);
				glUniform4fv(4, 1, &eye[0]);
				glUseProgram(0);
		}

		void IRenderer::SetCamera(const glm::mat4& pers, const glm::mat4& view, const glm::vec4& eye)
		{
				glUseProgram(mProgram.GetProgramID());
				glUniformMatrix4fv(0, 1, false, &pers[0][0]);
				glUniformMatrix4fv(1, 1, false, &view[0][0]);
				glUniform4fv(4, 1, &eye[0]);
				glUseProgram(0);
		}

		void IRenderer::SetModel(const glm::mat4& modelingMatrix)
		{
				glm::mat4 normal = glm::mat4(glm::mat3(affine_inverse(modelingMatrix)));

				glUseProgram(mProgram.GetProgramID());
				glUniformMatrix4fv(2, 1, false, &modelingMatrix[0][0]);
				glUniformMatrix4fv(3, 1, true, &normal[0][0]);
				glUseProgram(0);
		}

		void IRenderer::SetMaterial(const glm::vec3& diffuseCoeff, const glm::vec3& specularCoeff, float specularExponent)
		{
				glUseProgram(mProgram.GetProgramID());
				glUniform3fv(5, 1, &diffuseCoeff[0]);
				glUniform3fv(6, 1, &specularCoeff[0]);
				glUniform1fv(7, 1, &specularExponent);
				glUseProgram(0);
		}

		void IRenderer::CreateLight(const std::uint8_t lightID, const glm::vec3& position, const glm::vec3& color)
		{
				glUseProgram(mProgram.GetProgramID());

				int True = 1;

				const std::uint8_t lightLimit = 8;

				glUniform4fv(9 + lightID, 1, &position[0]);
				glUniform3fv(17 + lightID, 1, &color[0]);
				glUniform1iv(25 + lightID, 1, &True);

				glUseProgram(0);
		}

		void IRenderer::SetAmbientLight(const glm::vec3& color)
		{
				glUseProgram(mProgram.GetProgramID());
				glUniform3fv(8, 1, &color[0]);
				glUseProgram(0);
		}

		void IRenderer::DrawMesh(std::uint64_t meshID) const
		{
				const MeshData& md = mMeshDatas.at(meshID);
				constexpr std::uint64_t faceSize = sizeof(Mesh::Face) / sizeof(GLuint);

				glUseProgram(mProgram.GetProgramID());
				glBindVertexArray(md.mVertexArrayObject);

				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDrawElements(GL_TRIANGLES, faceSize * md.mFaceCount, GL_UNSIGNED_INT, 0);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				glBindVertexArray(0);
				glUseProgram(0);
		}

		GLuint IRenderer::LoadShader(GLenum shaderType, const std::filesystem::path& shaderPath)
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

		IRenderer::MeshData::MeshData()
				: mVertexArrayObject(num_max<GLuint>())
				, mVertexBuffer(num_max<GLuint>())
				, mNormalBuffer(num_max<GLuint>())
				, mFaceBuffer(num_max<GLuint>())
				, mFaceCount(num_max<size_t>())
				, mEdgeCount(num_max<size_t>())
		{
		}

		IRenderer::MeshData::~MeshData()
		{
		}

		void IRenderer::MeshData::GenVertexBuffer(const Mesh& mesh)
		{
				glGenBuffers(1, &mVertexBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
				glBufferData(GL_ARRAY_BUFFER, sizeof(Mesh::Vertex) * mesh.mVertices.size(), mesh.mVertices.data(), GL_STATIC_DRAW);
		}

		void IRenderer::MeshData::GenNormalBuffer(const NormalMesh& mesh)
		{
				glGenBuffers(1, &mNormalBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffer);
				glBufferData(GL_ARRAY_BUFFER, sizeof(NormalMesh::Normal) * mesh.mNormals.size(), mesh.mNormals.data(), GL_STATIC_DRAW);
		}

		void IRenderer::MeshData::GenFaceBuffer(const Mesh& mesh)
		{
				glGenBuffers(1, &mFaceBuffer);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mFaceBuffer);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Mesh::Face) * mesh.mFaces.size(), mesh.mFaces.data(), GL_STATIC_DRAW);

				mFaceCount = mesh.mFaces.size();
		}

		void IRenderer::MeshData::BindBuffers()
		{
				glGenVertexArrays(1, &mVertexArrayObject);
				glBindVertexArray(mVertexArrayObject);

				glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
				glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, 0);
				glEnableVertexAttribArray(0);

				glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffer);
				glVertexAttribPointer(1, 4, GL_FLOAT, false, 0, 0);
				glEnableVertexAttribArray(1);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mFaceBuffer);

				glBindVertexArray(0);
		}

		void IRenderer::MeshData::DeleteBuffers()
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
}
