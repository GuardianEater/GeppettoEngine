/*****************************************************************//**
 * \file   RelationSystem.hpp
 * \brief  system that handles relations between entities,
 *         for example when a parent moves and its children also move
 *
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#include "ISystem.hpp"

#pragma once

namespace Gep
{
    struct VQS;
}

namespace Client
{
    struct Transform;
}

namespace Gep::Event
{
    struct EntityAttached;
}

namespace Client
{
    class RelationSystem : public Gep::ISystem
    {
    public:
        RelationSystem(Gep::EngineManager& em)
            : ISystem(em)
        {
        }

        void Initialize() override;
        void Update(float dt) override;
        void UpdateRecursive(const Gep::Entity e, Transform& t, const Gep::VQS& parentTransform, bool parentDirty);
        void RecomputeLocalForNewParent(Gep::Entity child, Gep::Entity newParent);

        void OnEntityAttached(const Gep::Event::EntityAttached& event);
    };
}