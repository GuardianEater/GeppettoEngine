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
        Gep::FrameBuffer shadowMap = Gep::FrameBuffer::CreateDepthMap({ SHADOW_MAP_SIZE << 3, SHADOW_MAP_SIZE << 3});
    };
}
