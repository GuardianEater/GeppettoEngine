/*****************************************************************//**
 * \file   Archetypes.cpp
 * \brief  implementation for archetypes structure
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   November 2025
 *********************************************************************/

#include "pch.hpp"

#include "Archetypes.h"

namespace Gep
{
    size_t Archetype::EntityCount() const
    {
        return entityCount;
    }

    std::byte* Archetype::GetEntity(size_t x, size_t y)
    {
        return chunks[x].bytes.get() + stride * y;
    }

    const std::byte* Archetype::GetEntity(size_t x, size_t y) const
    {
        return chunks[x].bytes.get() + stride * y;
    }

    std::byte* Archetype::GetEntityAtBack()
    {
        const auto& chunk = chunks.back();

        return chunk.bytes.get() + stride * (chunk.count - 1);
    }

    const std::byte* Archetype::GetEntityAtBack() const
    {
        const auto& chunk = chunks.back();

        return chunk.bytes.get() + stride * (chunk.count - 1);
    }

    glm::u64vec2 Archetype::GetBackEntityIndex()
    {
        const size_t chunkIndex = chunks.size() - 1;
        const size_t bytesIndex = chunks[chunkIndex].count - 1;

        return glm::u64vec2(chunkIndex, bytesIndex);
    }

    std::byte* Archetype::GetComponent(std::byte* entity, size_t componentIndex)
    {
        uint64_t componentOffset = componentOffsets.at(componentIndex);
        return entity + componentOffset;
    }

    const std::byte* Archetype::GetComponent(const std::byte* entity, size_t componentIndex) const
    {
        uint64_t componentOffset = componentOffsets.at(componentIndex);
        return entity + componentOffset;
    }

    bool Archetype::InBounds(size_t x, size_t y) const
    {
        return (x < chunks.size() && y < chunks[x].count);
    }

    void Archetype::AllocateNewChunkAtBack(size_t capacity)
    {
        Chunk chunk{
            .bytes = std::make_unique<std::byte[]>(stride * capacity),
            .capacity = capacity,
            .count = 0
        };

        chunks.push_back(std::move(chunk));
    }

    void Archetype::DeallocateChunkAtBack()
    {
        if (chunks.empty())
            return;

        entityCount -= chunks.back().count;

        chunks.pop_back();
    }

    std::byte* Archetype::AllocateEntity()
    {
        if (chunks.empty())
        {
            AllocateNewChunkAtBack(FIRST_CHUNK_SIZE); // the amount of entities to store in the new chunk
        }
        else if (chunks.back().count >= chunks.back().capacity)
        {
            AllocateNewChunkAtBack(CHUNK_SIZE); // the amount of entities to store in the new chunk
        }

        Chunk& chunk = chunks.back();
        std::byte* entityPtr = chunk.bytes.get() + chunk.count * stride;
        chunk.count++;
        entityCount++;

        return entityPtr;
    }

    void Archetype::RemoveEntityAtBack()
    {
        if (chunks.empty())
            return;

        Chunk& chunk = chunks.back();

        chunk.count--;
        entityCount--;

        if (chunk.count == 0) // if there are no entities in this chunk remove it
        {
            DeallocateChunkAtBack();
        }
    }
}
