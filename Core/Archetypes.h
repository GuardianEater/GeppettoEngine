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
    struct ArchetypeChunk
    {
        Signature signature{0}; // the signature of the archetype
        size_t entityCount{0};  // the number of entities in this archetype
        size_t stride{0};       // the size of each entity including all of its components

        // [-------------stride------------]
        // [entity0][component0][component1] [entity1][component0][component1]
        std::vector<uint8_t> data{};
        std::array<size_t, MAX_COMPONENTS> componentOffsets{}; // given the global index of a component, gives the offset of that component in this chunk in bytes.
    };

    // each node represents a collection of archetypes, or a specific query. ie, a node could contain all <Transform,Mesh> archetypes
    struct ArchetypeChunkNode
    {
        Signature signature{}; // the query of components that this node matches

        ArchetypeChunkNode* parent = nullptr; // the parent of this node
        std::vector<size_t> archetypesIndecies{}; // all of the archetypes that exist in this node
        std::vector<ArchetypeChunkNode> children{}; // all of the children of this node, indexes correlate to component indexes
        size_t childrenCount = 0; // the amonut of children that are actively being used. used for deallocation
    };
}
