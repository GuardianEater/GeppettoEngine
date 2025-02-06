/*****************************************************************//**
 * \file   CameraComponent.hpp
 * \brief  Component that stores camera data
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <glm.hpp>

#include "IRenderTarget.hpp"
#include "RenderTargetImgui.hpp"

namespace Client
{
    struct Camera
    {
        glm::vec3 back  = glm::vec3(0, 0, 1);                                    // vector pointing out the back of the camera
        glm::vec3 right = glm::normalize(glm::cross(-back, glm::vec3(0, 1, 0))); // vector pointing to the right of the camera
        glm::vec3 up    = glm::cross(back, right);                               // vector pointer out the top of the camera

        float nearPlane = 1.0f;    // how far away to start rendering
        float farPlane  = 1000.0f; // how far away to stop rendering

        // the 80.f is the field of view
        // the 1.0f is the aspect ratio
        glm::vec3 viewport{
            2.0f * nearPlane * glm::tan(glm::radians(80.0f / 2.0f)),        // the width of the viewport
            2.0f * nearPlane * glm::tan(glm::radians(80.0f / 2.0f)) / 1.0f, // the height of the viewport
            nearPlane                                                       // the depth of the viewport
        };

        std::shared_ptr<Gep::IRenderTarget> renderTarget = std::make_shared<Gep::RenderTargetImgui>(500, 500);
    };
}
