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
    class IRenderTarget
    {
    public:
        IRenderTarget(int width, int height)
            : mSize(width, height)
        {}
        virtual ~IRenderTarget() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;
        virtual void Clear(const glm::vec3& color) = 0;
        virtual void Resize(glm::vec2 newSize) {};
        virtual void Draw() = 0;
        virtual glm::vec2 GetSize() const final { return mSize; }

    protected:
        glm::vec2 mSize;
    };
}

