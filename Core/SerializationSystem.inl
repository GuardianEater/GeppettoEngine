/*****************************************************************//**
 * \file   SerializationSystem.inl
 * \brief  template implementation of SerializationSystem
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include "SerializationSystem.hpp"

#include "EngineManager.hpp"

#include "Logger.hpp"

#include "nlohmann/json.hpp"

#include <rfl.hpp>
#include <rfl/json.hpp>
#include "reflect-cpp-extra.hpp"

namespace Client
{
    template <typename T>
    concept HasSerialize = requires(const T t)
    {
        { t.Serialize() } -> std::same_as<void>;
    };

    template <typename T>
    concept CanRead = requires(const T t)
    {
        rfl::json::read<T, rfl::DefaultIfMissing>("");
    };

    template <typename T, typename... Ps>
    concept CanWrite = requires(T t)
    {
        { rfl::json::write<Ps...>(t, 0) } -> std::same_as<std::string>;
    };

    template <typename... ComponentTypes>
    void SerializationSystem::OnComponentsRegistered(Gep::type_list<ComponentTypes...> componentTypes)
    {
        componentTypes.for_each([&]<typename ComponentType>()
        {
            mSaveComponentFunctions.push_back([&](Gep::Entity entity)
            {
                ComponentType& component = mManager.GetComponent<ComponentType>(entity);
                std::string componentName = Gep::GetTypeInfo<ComponentType>().PrettyName();

                nlohmann::json componentDataJson = nlohmann::json::object();
                const auto view = rfl::to_view(component);

                // this writes each type inside of the component
                view.apply([&](const auto& f)
                {
                    WriteType(componentDataJson, f.name(), *f.value());
                });

                nlohmann::json componentJson = nlohmann::json::object();
                componentJson["data"] = componentDataJson;
                componentJson["type"] = componentName;

                return componentJson;
            });
        });

        componentTypes.for_each([&]<typename ComponentType>()
        {
            std::string name = Gep::GetTypeInfo<ComponentType>().PrettyName();
            mLoadComponentFunctions[name] = [&](Gep::Entity entity, const nlohmann::json& componentDataJson)
            {
                const std::string componentDataStr = componentDataJson.dump();

                ComponentType component{};

                const auto view = rfl::to_view(component);
                view.apply([&](const auto& f)
                {
                    ReadType(componentDataJson, f.name(), *f.value());
                });

                mManager.AddComponent<ComponentType>(entity, component);
            };
        });
    }

    template<typename T>
    void SerializationSystem::WriteType(nlohmann::json& json, const std::string_view name, T& t)
    {
        json[name] = 0;
    }

    template <typename T>
    requires JsonWritable<T>
    void SerializationSystem::WriteType(nlohmann::json& json, const std::string_view name, T& t)
    {
        json[name] = t;
    }

    template<typename T>
    void SerializationSystem::ReadType(const nlohmann::json& json, const std::string_view name, T& t)
    {
        // do nothing if the type is not readable
    }

    template <typename T>
        requires JsonReadable<T>
    void SerializationSystem::ReadType(const nlohmann::json& json, const std::string_view name, T& t)
    {
        t = json[name].get<std::remove_reference_t<T>>();
    }
}
