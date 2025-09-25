/*****************************************************************//**
 * \file   Mesh.hpp
 * \brief  Mesh data structure used when rendering
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "Shapes.hpp"

namespace Gep
{
    // based on International Journal of Algebra, Vol. 2, 2008, no. 19,905 - 918
    // VQS-transformation basically scales r by s, rotates the outcome by q, and then
    // translates the latter by v.
    struct VQS // VQM
    {
        glm::vec3 position{0,0,0}; // translation
        glm::quat rotation{1,0,0,0}; // rotation
        float scale{1.0f}; // uniform scaling factor

        VQS operator*(const VQS& local) const 
        {
            VQS result{};
            result.scale = scale * local.scale;
            result.rotation = rotation * local.rotation;

            glm::vec3 scaledLocalTrans = scale * local.position;
            glm::vec3 rotatedLocalTrans = rotation * scaledLocalTrans;
            result.position = position + rotatedLocalTrans;

            return result;
        }
    };

    struct Vertex
    {
        glm::vec3 position{};
        glm::vec3 normal{};
        glm::vec2 texCoord{};
        std::array<uint64_t, 4> boneIndices{};
        std::array<float, 4> boneWeights{};
    };

    struct Mesh
    {
        std::string name = "Unnamed";

        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        uint32_t materialIndex{}; // the material inside of model that this 
        std::vector<uint32_t> boneIndices{}; // all of the bones that this mesh impacts in the model struct
        AABB boundingBox{};

        void CalculateBoundingBox()
        {
            boundingBox.min = glm::vec3(FLT_MAX);
            boundingBox.max = glm::vec3(-FLT_MAX);
            for (const Vertex& vertex : vertices)
            {
                boundingBox.min = glm::min(boundingBox.min, vertex.position);
                boundingBox.max = glm::max(boundingBox.max, vertex.position);
            }
        }

        // normalizes the mesh to fit within a unit cube, also calculates the bounding box
        void Normalize()
        {
            CalculateBoundingBox();

            glm::vec3 center = (boundingBox.min + boundingBox.max) * 0.5f;
            glm::vec3 size   = boundingBox.max - boundingBox.min;

            float maxDim = glm::max(size.x, glm::max(size.y, size.z));
            for (Vertex& vertex : vertices)
            {
                vertex.position = (vertex.position - center) / maxDim;
            }
        }
    };

    struct Bone
    {
        std::string name = "Unnamed";

        std::vector<uint16_t> childrenIndices;
        VQS transformation;
        VQS inverseBind;
        uint16_t parentIndex;
    };

    struct Skeleton
    {
        std::vector<Bone> bones; // must always be sorted
    };

    struct KeyFrame
    {
        float time; // time since the start of the animation
        VQS toParentFromBone;
    };

    struct Track
    {
        std::vector<KeyFrame> keyFrames;
    };

    struct Animation
    {
        float duration; // total duration of the animations
        std::vector<Track> tracks; // one track per bone
    };
}

