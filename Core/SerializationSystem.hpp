/*****************************************************************//**
 * \file   SerializationSystem.hpp
 * \brief  Saves and loads the state of the engine
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
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

#include "nlohmann/json.hpp"

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
        void OnComponentsRegistered(Gep::type_list<ComponentTypes...> componentTypes);

        void OnAssetBrowserItemClicked(const Gep::Event::AssetBrowserItemClicked& event);
        void OnEngineStateChanged(const Gep::Event::StateChanged& event);

        void Initialize() override;
        void Exit() override;

    private:

        // component index -> function that converts component to json
        std::vector<std::function<nlohmann::json(Gep::Entity)>> mSaveComponentFunctions;
        std::map<std::string, std::function<void(Gep::Entity, const nlohmann::json&)>> mLoadComponentFunctions;
    };
}

#include "SerializationSystem.inl"