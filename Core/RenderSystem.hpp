/*****************************************************************//**
 * \file   RenderSystem.hpp
 * \brief  System that renders objects
 * 
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

// core
#include <Core.hpp>

// backend
#include <System.hpp>
#include <EngineManager.hpp>
#include <Renderer.hpp>
#include <SphereMesh.hpp>

// client
#include <Transform.hpp>
#include <Material.hpp>

namespace Client
{
	class RenderSystem : public Gep::ISystem
	{
	public:
		RenderSystem(Gep::EngineManager& em)
			: ISystem(em)
			, mRenderer()
			, mCamera(glm::vec4(0, 0, 0, 1) + 10.f * glm::vec4(0, 0, 1, 0), -glm::vec4(0, 0, 1, 0), glm::vec4(0, 1, 0, 0), 80, 1, 0.1f, 100)
			, mSphereMesh()
		{
			mRenderer.LoadVertexShader("assets\\shaders\\PhongRender.vert");
			mRenderer.LoadFragmentShader("assets\\shaders\\PhongRender.frag");
			mRenderer.Compile();

			mRenderer.BackfaceCull();
			mRenderer.SetAmbientLight({ 0.5, 0.5, 0.5 });

			//temporary
			mSphereMesh = mRenderer.LoadMesh(Gep::SphereMesh(10, 5));

			Gep::Camera camera();
		}

		~RenderSystem()
		{
			mRenderer.UnloadMesh(mSphereMesh);
		}

		void Init() override
		{
			mRenderer.CreateLight(0, { 0, 5, 0 }, { 1, 0, 0 });
		}

		void Update(float dt) override
		{
			mRenderer.Clear();

			mRenderer.SetCamera(mCamera);

			for (Gep::Entity entity : mEntities)
			{
				Transform& transform = mManager.GetComponent<Transform>(entity);
				Material& material = mManager.GetComponent<Material>(entity);

				const glm::mat4 model = Gep::scale_matrix(transform.scale)
									  * Gep::rotation_matrix(transform.rotationAmount, { transform.rotationAxis, 0 }) 
									  * Gep::translation_matrix(transform.position);

				mRenderer.SetModel(model);
				mRenderer.SetMaterial(material.diff_coeff, material.spec_coeff, material.spec_exponent);
				mRenderer.DrawMesh(material.meshID);
			}
		}

	private:
		Gep::IRenderer mRenderer;
		Gep::Camera mCamera;

		size_t mSphereMesh;
	};
}
