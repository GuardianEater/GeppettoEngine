/*****************************************************************//**
 * \file   ShadowCasterComponent.hpp
 * \brief  component that can be added that will cause an attached light component to cast shadows
 * 
 * \author 2018t
 * \date   February 2026
 *********************************************************************/

#pragma once

#include "FrameBuffer.hpp"

namespace Client
{
    static constexpr size_t SHADOW_MAP_SIZE = 1 << 9;

    struct ShadowCasterComponent
    {
        Gep::FrameBuffer shadowMap = Gep::FrameBuffer::CreateDepthCubeMap({ SHADOW_MAP_SIZE, SHADOW_MAP_SIZE });
    };

    struct ShadowCasterDirectionalComponent
    {
        Gep::FrameBuffer shadowMap = Gep::FrameBuffer::CreateWithTexture({ SHADOW_MAP_SIZE, SHADOW_MAP_SIZE }, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
    };
}
