/*****************************************************************//**
 * \file   EntityManager.hpp
 * \brief  Manages entities such as constuction and destruction
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

namespace Gep
{
    class EntityManager
    {
    public:
        EntityManager() 
            : mAvailableEntities()
            , mSignatures()
        {
            // fills the entity vector with ids that can be used
            for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) 
            {
                mAvailableEntities.push_back(entity);
            }
        }

        Entity CreateEntity() 
        {
            Entity id = mAvailableEntities.back();
            mAvailableEntities.pop_back();

            return id;
        }

        void SetSignature(Entity entity, Signature signature)
        {
            // Put this entity's signature into the array
            mSignatures[entity] = signature;
        }

        Signature GetSignature(Entity entity)
        {
            // Put this entity's signature into the array
            return mSignatures[entity];
        }

        void DestroyEntity(Entity entity) 
        {
            mSignatures[entity].reset();

            mAvailableEntities.push_back(entity);
        }

    private:
        std::vector<Entity> mAvailableEntities;

        // this keeps track of which components an entity has
        std::array<Signature, MAX_ENTITIES> mSignatures;
    };
}