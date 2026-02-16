/*****************************************************************//**
 * \file   ShadowCasterComponent.hpp
 * \brief  component that can be added that will cause an attached light component to cast shadows
 * 
 * \author 2018t
 * \date   February 2026
 *********************************************************************/

#pragma once

#include "FrameBuffer.hpp"
#include "glew.h"

namespace Client
{
    struct ShadowCasterComponent
    {
        Gep::FrameBuffer shadowFrameBuffer = Gep::FrameBuffer::CreateDepthCubeMap({ 1 << 9, 1 << 9 });
    };
}
