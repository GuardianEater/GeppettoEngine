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
#include <GLFW/glfw3.h>

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\ext\matrix_clip_space.hpp>

#include <Affine.hpp>

namespace Gep
{
    class Camera
    {
    public:
        Camera(const glm::vec4& position, const glm::vec4& lookAt, const glm::vec4& relativeUp, float fov, float aspect, float nearDistance, float farDistance);

        // getter /////////////////////////////////////////////////////////////////////////////////

        const glm::vec4& GetEyePosition() const;
        const glm::vec4& GetBackVector() const;
        const glm::vec4& GetUpVector() const;
        const glm::vec4& GetRightVector() const;
        const glm::vec3& GetViewport() const;
        float GetNearPlaneDistance() const;
        float GetFarPlaneDistance() const;
        glm::mat4 GetPerspective() const;
        glm::mat4 GetModel() const;
        glm::mat4 GetView() const;

        // incrementers ///////////////////////////////////////////////////////////////////////////

        void Zoom(float amount);
        void Forward(float amount);
        void RotateYaw();

    private:
        glm::vec4 mEyePoint;
        glm::vec3 mViewport;
        float mNearPlane;
        float mFarPlane;
        glm::vec4 mBack;
        glm::vec4 mRight;
        glm::vec4 mUp;
    };
}
