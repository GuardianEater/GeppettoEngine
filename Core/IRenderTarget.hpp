/*****************************************************************//**
 * \file   IRenderTarget.hpp
 * \brief  contained by a camera, the camera will render to this target
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include "glew.h"

namespace Gep
{
    #pragma once
    class IRenderTarget
    {
    public:
        IRenderTarget(int width, int height)
            : mWidth(width)
            , mHeight(height)
        {}
        virtual ~IRenderTarget() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;
        virtual void Clear() = 0;
        virtual void Resize(int width, int height) = 0;
        virtual void Draw() = 0;

    protected:
        int mWidth;
        int mHeight;
    };
}

