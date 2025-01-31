/*****************************************************************//**
 * \file   SerializationSystem.hpp
 * \brief  Saves and loads the state of the engine
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include "ISystem.hpp"
#include "TypeID.hpp"
#include "TypeList.hpp"
#include "EngineManager.hpp"
#include "Logger.hpp"

#include <fstream>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include "reflect-cpp-extra.hpp"

namespace Client
{
    class SerializationSystem : public Gep::ISystem
    {
    private:

    public:
        SerializationSystem(Gep::EngineManager& em)
            : ISystem(em)
        {}

        template <typename... ComponentTypes>
        void OnComponentsRegistered(Gep::type_list<ComponentTypes...> componentTypes)
        {
            componentTypes.for_each([&]<typename ComponentType>()
            {
                mSaveComponentFunctions.push_back([&](Gep::Entity entity)
                {
                    ComponentType& component = mManager.GetComponent<ComponentType>(entity);

                    std::string componentName = Gep::GetTypeInfo<ComponentType>().PrettyName();
                    
                    std::string json = rfl::json::write(component);

                    Gep::Log::Info(componentName, ": ", json);
                });
            });
        }

        void Exit() override
        {
            std::vector<Gep::Entity>& entities = mManager.GetEntities();

            for (Gep::Entity entity : entities)
            {
                Gep::Log::Info("Saving Entity: [", entity, "]");

                mManager.ForEachComponent(entity, [&](const Gep::ComponentData& componentData)
                {
                    mSaveComponentFunctions[componentData.index](entity);
                });
            }

            //std::ofstream file("save.json");
        }

    private:

        // component index -> function that saves the component
        std::vector<std::function<void(Gep::Entity)>> mSaveComponentFunctions;
    };
}
