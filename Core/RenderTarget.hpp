/*****************************************************************//**
 * \file   FrameBuffer.hpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>

namespace Gep
{
    class FrameBuffer
    {
    public:
        static FrameBuffer Create(const glm::ivec2 size);

        void Bind() const;
        void Unbind() const;
        void Clear(const glm::vec4& color = { 0.0f, 0.0f, 0.0f, 0.0f }) const;
        void Resize(glm::ivec2 newSize);
        void UpdateViewport() const;

        glm::ivec2 GetSize() const { return mSize; }

    private:
        struct TargetData
        {
            GLuint frameBuffer = 0;
            GLuint texture = 0;
            GLuint depthBuffer = 0;

            ~TargetData()
            {
                glDeleteFramebuffers(1, &frameBuffer);
                glDeleteTextures(1, &texture);
                glDeleteRenderbuffers(1, &depthBuffer);
            }
        };

    private:
        // this is so if the frame buffer is copied it will still reference the same underlying data, also avoids delete issues
        std::shared_ptr<TargetData> mTarget;

        glm::ivec2 mSize = { 0, 0 };
    };
}
