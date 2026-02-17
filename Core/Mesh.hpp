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

    struct Mesh
    {
        std::string name = "Unnamed";

        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        std::vector<uint32_t> materials{}; 
        std::vector<uint32_t> boneIndices{}; // all of the bones that this mesh impacts in the model struct
        AABB boundingBox{};

        GLuint vao = 0;
        GLuint vBuffer = 0;
        GLuint iBuffer = 0;
        size_t commitIndexCount = 0; // the number of indices that have been committed to the gpu, used for drawing

        Skeleton skeleton{}; // if this mesh is animated, it will have a skeleton. if not, this will be null
        uint64_t animationID = Gep::NumMax<uint64_t>();

        void Create()
        {
            Destroy(); // in case this mesh already has buffers, destroy them before creating new ones

            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vBuffer);
            glGenBuffers(1, &iBuffer);
        }

        void Commit()
        {
            glBindVertexArray(vao);
    
            glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
    
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * indices.size(), indices.data(), GL_STATIC_DRAW);
            commitIndexCount = indices.size();
    
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
            glEnableVertexAttribArray(0);
    
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
            glEnableVertexAttribArray(1);
    
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
            glEnableVertexAttribArray(2);
    
            glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIndices));
            glEnableVertexAttribArray(3);
    
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));
            glEnableVertexAttribArray(4);
    
            glBindVertexArray(0);
        }

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

        void Destroy()
        {
            if (vao != 0)
                glDeleteVertexArrays(1, &vao);
            if (vBuffer != 0)
                glDeleteBuffers(1, &vBuffer);
            if (iBuffer != 0)
                glDeleteBuffers(1, &iBuffer);
            vao = 0;
            vBuffer = 0;
            iBuffer = 0;
            commitIndexCount = 0;
        }

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        Mesh()
        {
            Create();
        }

        Mesh(Mesh&& other) noexcept
            : name(std::move(other.name))
            , vertices(std::move(other.vertices))
            , indices(std::move(other.indices))
            , materials(std::move(other.materials))
            , boneIndices(std::move(other.boneIndices))
            , boundingBox(other.boundingBox)
            , vao(other.vao)
            , vBuffer(other.vBuffer)
            , iBuffer(other.iBuffer)
            , commitIndexCount(other.commitIndexCount)
            , skeleton(std::move(other.skeleton))
            , animationID(other.animationID)
        {
            other.vao = 0;
            other.vBuffer = 0;
            other.iBuffer = 0;
            other.commitIndexCount = 0;
            other.animationID = Gep::NumMax<uint64_t>();
        }

        Mesh& operator=(Mesh&& other) noexcept
        {
            if (this != &other)
            {
                Destroy(); // destroy whatever is in the current mesh before moving the new data in
                name = std::move(other.name);
                vertices = std::move(other.vertices);
                indices = std::move(other.indices);
                materials = std::move(other.materials);
                boneIndices = std::move(other.boneIndices);
                boundingBox = other.boundingBox;
                vao = other.vao;
                vBuffer = other.vBuffer;
                iBuffer = other.iBuffer;
                commitIndexCount = other.commitIndexCount;
                skeleton = std::move(other.skeleton);
                animationID = other.animationID;

                // invalidate the other meshs handles
                other.vao = 0;
                other.vBuffer = 0;
                other.iBuffer = 0;
                other.commitIndexCount = 0;
                other.animationID = Gep::NumMax<uint64_t>();
            }

            return *this;
        }

        ~Mesh()
        {
            Destroy();
        }
    };
}

