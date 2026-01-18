/*****************************************************************//**
 * \file   Mesh.hpp
 * \brief  Mesh data structure used when rendering
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include "VQS.hpp"
#include "Shapes.hpp"
#include "Core.hpp"
#include <limits>
#include <array>

namespace Gep
{
    struct Vertex
    {
        constexpr static const uint32_t INVALID_INDEX = Gep::NumMax<uint32_t>();

        glm::vec3 position{};
        glm::vec3 normal{};
        glm::vec2 texCoord{};
        std::array<uint32_t, 4> boneIndices{ INVALID_INDEX , INVALID_INDEX, INVALID_INDEX, INVALID_INDEX };
        std::array<float, 4> boneWeights{ 0.25f, 0.25f, 0.25f, 0.25f };
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
                boundingBox.min = (glm::min)(boundingBox.min, vertex.position);
                boundingBox.max = (glm::max)(boundingBox.max, vertex.position);
            }
        }

        // normalizes the mesh to fit within a unit cube, also calculates the bounding box
        void Normalize()
        {
            CalculateBoundingBox();

            glm::vec3 center = (boundingBox.min + boundingBox.max) * 0.5f;
            glm::vec3 size   = boundingBox.max - boundingBox.min;

            float maxDim = (glm::max)(size.x, (glm::max)(size.y, size.z));
            for (Vertex& vertex : vertices)
            {
                vertex.position = (vertex.position - center) / maxDim;
            }
        }
    };

    struct Bone
    {
        std::string name = "Unnamed";

        std::vector<uint32_t> childrenIndices;
        Gep::VQS transformation;
        Gep::VQS inverseBind;
        uint32_t parentIndex;

        bool isRealBone = false; // wether or not this bone was a bone inside of assimp
    };

    struct Skeleton
    {
        std::vector<Bone> bones; // must always be sorted
    };

    template <typename Type>
    struct KeyFrame
    {
        float time = 0.0f; // time since the start of the animation
        Type transform{};
    };

    struct Track
    {
        uint32_t boneIndex; // refers to a specific bone

        std::vector<KeyFrame<glm::vec3>> positionKeyFrames;
        std::vector<KeyFrame<glm::quat>> rotationKeyFrames;
        std::vector<KeyFrame<glm::vec3>> scaleKeyFrames;
    };

    struct Animation
    {
        std::string name;
        float duration = 0.0f; // total duration of the animation in TICKS
        float ticksPerSecond = 0.0f; // how many ticks that should pass in a second
        std::vector<Track> tracks; // one track per bone
    };
}

