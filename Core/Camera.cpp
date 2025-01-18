/*****************************************************************//**
 * \file   Camera.cpp
 * \brief  Implementation for a camera
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "Camera.hpp"

namespace Gep
{
    Camera::Camera(const glm::vec4& position, const glm::vec4& lookAt, const glm::vec4& relativeUp, float fov, float aspect, float nearDistance, float farDistance)
        : mEyePoint(position)
        , mViewport({ 0, 0, nearDistance })
        , mNearPlane(nearDistance)
        , mFarPlane(farDistance)
        , mBack(-glm::normalize(lookAt))
        , mRight(glm::normalize(cross_product(lookAt, relativeUp))) // crosses vec4's
        , mUp(cross_product(mBack, mRight))
    {
        // near plane width/height
        mViewport.x = (2.0f) * (mViewport.z) * tanf(glm::radians(fov / 2.0f));
        mViewport.y = mViewport.x / aspect;
    }

    const glm::vec4& Camera::GetEyePosition() const
    {
        return mEyePoint;
    }

    const glm::vec4& Camera::GetBackVector() const
    {
        return mBack;
    }

    const glm::vec4& Camera::GetUpVector() const
    {
        return mUp;
    }

    const glm::vec4& Camera::GetRightVector() const
    {
        return mRight;
    }

    const glm::vec3& Camera::GetViewport() const
    {
        return mViewport;
    }

    float Camera::GetNearPlaneDistance() const
    {
        return mNearPlane;
    }

    float Camera::GetFarPlaneDistance() const
    {
        return mFarPlane;
    }

    glm::mat4 Camera::GetPerspective() const
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

    glm::mat4 Camera::GetModel() const
    {
        return glm::mat4(mRight, mUp, mBack, mEyePoint);
    }

    glm::mat4 Camera::GetView() const
    {
        return affine_inverse(GetModel());
    }

    void Camera::Zoom(float amount)
    {
        mViewport *= amount;
    }

    void Camera::Forward(float amount)
    {
        mEyePoint -= (amount * glm::normalize(mBack));
    }

    void Camera::RotateYaw()
    {
        // Implementation for RotateYaw
    }
}
