/*****************************************************************//**
 * \file   Manager.hpp
 * \brief  
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

// core
#include <Core.hpp>

namespace Gep
{
    class ComponentManager // something that contains components
    {
    public:
        ComponentManager()
        {}

        ~ComponentManager() = default;

    public:
        template <typename ComponentType>
        void Register()
        {
            
        }

        template<typename ComponentType>
        ComponentBitPos GetComponentBitPos() 
        {
            
        }

        template<typename ComponentType>
        void AddComponent(Entity entity, const ComponentType& component)
        {
            
        }

        template<typename ComponentType>
        void RemoveComponent(Entity entity)
        {
            GetComponentArray<ComponentType>()->Erase(entity);
        }



        template<typename ComponentType>
        ComponentType& GetComponent(Entity entity)
        {
            
        }

        void Event_EntityDestroyed(Entity entity)
        {
            
        }

    private:
        

    private:


    };
}