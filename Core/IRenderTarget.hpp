/*****************************************************************//**
 * \file   IRenderTarget.hpp
 * \brief  contained by a camera, the camera will render to this target
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include "glew.h"
#include "Core.hpp"

namespace Gep
{
    class EngineManager;

    class IRenderTarget
    {
    public:
        IRenderTarget(int width, int height)
            : mSize(width, height)
            , mPosition(0, 0)
        {}
        virtual ~IRenderTarget() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;
        virtual void Clear(const glm::vec3& color) = 0;
        virtual void Resize(glm::vec2 newSize) {};
        virtual void Draw(EngineManager& em, Entity camera, const std::function<void()>& drawFunction = [](){}) = 0;
        virtual glm::vec2 GetSize() const final { return mSize; }
        virtual glm::vec2 GetPosition() const final { return mPosition; }

    protected:
        glm::vec2 mSize{};
        glm::vec2 mPosition{};
    };
}

