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
    }
}
