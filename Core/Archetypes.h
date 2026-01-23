/*****************************************************************//**
 * \file   Archetypes.h
 * \brief  efficient storage of components
 *
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   April 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"

namespace Gep
{
    class Archetype
    {
    public:
        static constexpr size_t FIRST_CHUNK_SIZE = 16; // the amount of entities stored in the first allocated chunk
        static constexpr size_t CHUNK_SIZE = FIRST_CHUNK_SIZE * 64; // amount of entities stored in all other chunks

        Signature signature = 0; // the signature of the archetype
        size_t stride = 0;       // the size of each entity including all of its components

        std::array<size_t, MAX_COMPONENTS> componentOffsets{}; // given the global index of a component, gives the offset of that component in this chunk in bytes.

    private:
        struct Chunk
        {
            std::unique_ptr<std::byte[]> bytes;
            size_t capacity = 0;
            size_t count = 0;
        };

        size_t entityCount = 0;  // the number of entities in this archetype

        // [-------------stride------------]
        // [entity0][component0][component1] [entity1][component0][component1]
        std::vector<Chunk> chunks{};

    public:
        // gets the amount of entities total in this archetype
        size_t EntityCount() const;

        // gets an entity at its 2d index, x is index into the chunks vector and y is an index into the entities stored in bytes. 
        std::byte* GetEntity(size_t x, size_t y);
        const std::byte* GetEntity(size_t x, size_t y) const;

        // gets the entity at the very back
        std::byte* GetEntityAtBack();
        const std::byte* GetEntityAtBack() const;

        glm::u64vec2 GetBackEntityIndex();

        // gets the component for a specific entity given its component index
        std::byte* GetComponent(std::byte* entity, size_t componentIndex);
        const std::byte* GetComponent(const std::byte* entity, size_t componentIndex) const;

        // check for whether the given index exists
        bool InBounds(size_t x, size_t y) const;

        // pushes back an new chunk into the chunks vector with the given size
        void AllocateNewChunkAtBack(size_t capacity);

        // simply deallocates the back most chunk
        void DeallocateChunkAtBack();

        //  allocates an entity. if there is no space will allocate another chunk and use it automatically
        std::byte* AllocateEntity();

        // removes the entity
        void RemoveEntityAtBack();

        template <typename Func>
        requires std::is_invocable_v<Func, std::byte*>
        void ForEachEntity(const Func& func);
    };

    template<typename Func>
        requires std::is_invocable_v<Func, std::byte*>
    __forceinline void Archetype::ForEachEntity(const Func& func)
    {
        for (Chunk& chunk : chunks) //iterate over all of the chunks
        {
            std::byte* entity = chunk.bytes.get();
            std::byte* tail = entity + chunk.count * stride;

            for (; entity != tail; entity += stride) // iterate over all entities in chunk
            {
                func(entity);
            }
        }
    }
}
