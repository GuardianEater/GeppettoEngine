/*****************************************************************//**
 * \file   RenderTargetTexture.hpp
 * \brief  render target that renders to a texture
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include "IRenderTarget.hpp"

namespace Gep
{
    class RenderTargetTexture : public IRenderTarget
    {
    public:
        RenderTargetTexture(int width, int height);
        virtual ~RenderTargetTexture() override;

        void Bind() final override;
        void Unbind() final override;
        void Clear(const glm::vec3& color) final override;
        void Resize(glm::vec2 newSize) final override;
        GLuint GetTexture() const;

        virtual void Draw(EngineManager& em, Entity camera) override;

    private:
        GLuint mFBO;
        GLuint mTexture;
        GLuint mRBO;
    };
}
