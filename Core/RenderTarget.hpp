/*****************************************************************//**
 * \file   RenderTarget.hpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>

namespace Gep
{
    class RenderTarget
    {
    public:
        static RenderTarget Generate(glm::ivec2 size);

        void Bind() const;
        void Unbind() const;
        void Clear(const glm::vec3& color = { 0.0f, 0.0f, 0.0f }) const;
        void Resize(glm::ivec2 newSize);

        glm::ivec2 GetSize() const { return mSize; }

    private:
        struct TargetData
        {
            GLuint mFrameBuffer = 0;
            GLuint mTexture = 0;
            GLuint mRenderBuffer = 0;

            ~TargetData()
            {
                glDeleteFramebuffers(1, &mFrameBuffer);
                glDeleteTextures(1, &mTexture);
                glDeleteRenderbuffers(1, &mRenderBuffer);
            }
        };

    private:
        std::shared_ptr<TargetData> mTarget; 

        glm::ivec2 mSize = { 0.0f, 0.0f };
    };
}
