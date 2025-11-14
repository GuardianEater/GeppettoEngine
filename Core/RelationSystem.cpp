/*****************************************************************//**
 * \file   RelationSystem.cpp
 * \brief  handles relations between entities
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"
#include "RelationSystem.hpp"
#include "EngineManager.hpp"
#include "Transform.hpp"

namespace Client
{
    void RelationSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::EntityAttached>(this, &RelationSystem::OnEntityAttached);
    }

    void RelationSystem::Update(float dt)
    {
        mManager.ForEachArchetype<Transform>([&](Gep::Entity entity, Transform& t)
            {
                // only root entities
                if (!mManager.HasParent(entity))
                {
                    UpdateRecursive(entity, t, glm::mat4(1.0f), false);
                }
            });

    }

    void RelationSystem::UpdateRecursive(const Gep::Entity e, Transform& t, const glm::mat4& parentWorld, bool parentDirty)
    {
        bool worldDirty = parentDirty || t.dirty;

        if (worldDirty)
        {
            t.localM = Gep::ToMat4(t.local);
            t.worldM = parentWorld * t.localM;
        }

        t.dirty = false;

        if (mManager.HasChild(e))
        {
            for (Gep::Entity child : mManager.GetChildren(e))
            {
                Transform& childT = mManager.GetComponent<Transform>(child);
                UpdateRecursive(child, childT, t.worldM, worldDirty);
            }
        }
    }

    void RelationSystem::RecomputeLocalForNewParent(Gep::Entity child, Gep::Entity newParent)
    {
        // both must have transform
        if (!mManager.HasComponent<Transform>(child)) return;
        if (!mManager.HasComponent<Transform>(newParent)) return;

        Transform& childT = mManager.GetComponent<Transform>(child);
        Transform& parentT = mManager.GetComponent<Transform>(newParent);

        childT.localM = Gep::Inverse(parentT.worldM) * childT.worldM;
        childT.local = Gep::ToVQS(childT.localM);

        childT.dirty = true; // let Update() rebuild .world with new parent
    }

    void RelationSystem::OnEntityAttached(const Gep::Event::EntityAttached& event)
    {
        RecomputeLocalForNewParent(event.child, event.parent);
    }


}
