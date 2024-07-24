/*****************************************************************//**
 * \file   Camera.hpp
 * \brief  Implementation for a camera 
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include <glew.h>

#include <glm.hpp>
#include <GLFW/glfw3.h>

#include <matrix_transform.hpp>
#include <matrix_clip_space.hpp>

namespace Gep
{
	class Camera
	{
	public:
		Camera(const glm::vec4& position, const glm::vec4& lookAt, const glm::vec4& relativeUp, float fov, float aspect, float nearDistance, float farDistance)
			: mEyePoint(position)
			, mViewport({0, 0, nearDistance})
			, mNearPlane(nearDistance)
			, mFarPlane(farDistance)
			, mBack(-glm::normalize(lookAt))
			, mRight(glm::vec4{glm::normalize(glm::cross(glm::vec3(lookAt),glm::vec3(relativeUp))),0}) // crosses vec4's
			, mUp(glm::vec4{ glm::normalize(glm::cross(glm::vec3(mBack),glm::vec3(mRight))),0 })
		{
			// near plane width/height
			mViewport.x = (2.0f) * (mViewport.z) * tanf(glm::radians(fov / 2.0f));
			mViewport.y = mViewport.x / aspect;
		}

		// getter /////////////////////////////////////////////////////////////////////////////////

		const glm::vec4& GetEyePosition() const
		{
			return mEyePoint;
		}
		const glm::vec4& GetBackVector() const 
		{
			return mBack;
		}
		const glm::vec4& GetUpVector() const 
		{
			return mUp;
		}
		const glm::vec4& GetRightVector() const 
		{
			return mRight;
		}
		const glm::vec3& GetViewport() const 
		{
			return mViewport;
		}
		float GetNearPlaneDistance() const
		{
			return mNearPlane;
		}
		float GetFarPlaneDistance() const
		{
			return mFarPlane;
		}

		glm::mat4 GetPerspective() const
		{
			glm::mat4 result(0);

			// this formula creates the capital pi matrix
			result[0][0] = (2.0f * mViewport.z) / mViewport.x;
			result[1][1] = (2.0f * mViewport.z) / mViewport.y;
			result[2][2] = (mNearPlane + mFarPlane) / (mNearPlane - mFarPlane);
			result[3][2] = (2.0f * mNearPlane * mFarPlane) / (mNearPlane - mFarPlane);
			result[2][3] = -1;

			return result;
		}

		glm::mat4 GetModel() const
		{
			return glm::mat4(mRight, mUp, mBack, mEyePoint);
		}

		glm::mat4 GetView() const
		{
			return glm::inverse(GetModel());
		}

		// incrementers ///////////////////////////////////////////////////////////////////////////

		// zooms in the camera by an amount
		void Zoom(float amount)
		{
			mViewport *= amount;
		}

		// moves the camera forward by an amount
		void Forward(float amount)
		{
			mEyePoint -= (amount * glm::normalize(mBack));
		}

		// rotates around the up vector
		void RotateYaw()
		{
			
		}

	private:

	private:
		// the location where the eye is
		glm::vec4 mEyePoint;

		// camera specifications
		glm::vec3 mViewport; // x->width y->height z->Distance
		float mNearPlane;
		float mFarPlane;

		// the orientation of the camera
		glm::vec4 mBack;
		glm::vec4 mUp;
		glm::vec4 mRight;
	};
}
