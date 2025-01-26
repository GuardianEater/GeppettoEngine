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
                mComponentInspectorPanels[mManager.GetComponentSignature<ComponentType>()] = [&](Gep::Entity entity)
                    {
                        Gep::Log::Info("Saving Component: [", Gep::GetTypeInfo<ComponentType>().PrettyName(), "]");
                        const ComponentType& component = mManager.GetComponent<ComponentType>(entity);

                        //std::string json = rfl::json::write(component);
                    };
            });
        }

        void Exit() override
        {
            std::vector<Gep::Entity>& entities = mManager.GetEntities();

            for (Gep::Entity entity : entities)
            {
                Gep::Log::Info("Saving Entity: [", entity, "]");

                //rapidjson::Value entityValue(rapidjson::kObjectType);

                std::vector<Gep::Signature> componentSignatures = mManager.GetComponentSignatures(entity);
                for (const Gep::Signature componentSignature : componentSignatures)
                {
                    mComponentInspectorPanels[componentSignature](entity);
                }
            }

            //std::ofstream file("save.json");
        }

    private:
        std::unordered_map<Gep::Signature, std::function<void(Gep::Entity)>> mComponentInspectorPanels;
    };
}
