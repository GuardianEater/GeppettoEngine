/*****************************************************************//**
 * \file   CameraComponent.hpp
 * \brief  Component that stores camera data
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <glm\glm.hpp>

#include "IRenderTarget.hpp"
#include "RenderTargetImgui.hpp"
#include "RenderTargetWindow.hpp"

#include "TypeID.hpp"

namespace Client
{
    struct Camera
    {
        glm::vec3 back  = glm::vec3(0, 0, 1);                                    // vector pointing out the back of the camera
        glm::vec3 right = glm::normalize(glm::cross(-back, glm::vec3(0, 1, 0))); // vector pointing to the right of the camera
        glm::vec3 up    = glm::cross(back, right);                               // vector pointer out the top of the camera

        glm::vec3 rotation; // yaw pitch roll

        float nearPlane = 1.0f;    // how far away to start rendering
        float farPlane  = 1000.0f; // how far away to stop rendering
        float fov       = 80.0f;   // field of view
        float aspect    = 1.0f;    // aspect ratio

        std::shared_ptr<Gep::IRenderTarget> renderTarget = std::make_shared<Gep::RenderTargetImgui>(500, 500);
        Gep::TypeInfo renderTargetType = Gep::GetTypeInfo<Gep::RenderTargetImgui>();

        void Resize(glm::vec2 size)
        {
            aspect = size.x / size.y;
        }

        glm::mat4 GetViewMatrix(const glm::vec3& position)
        {
            return glm::lookAt(position, position - glm::normalize(back), up);
        }

        glm::mat4 GetProjectionMatrix()
        {
            //return Gep::perspective(viewport, nearPlane, farPlane);
            return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
        }
    };
}
