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

        void Initialize() override;
        void Exit() override;

        nlohmann::json SaveScene() const;
        void LoadScene(const nlohmann::json& sceneJson) const;

    private:

        // component index -> function that converts component to json
        std::vector<std::function<nlohmann::json(Gep::Entity)>> mSaveComponentFunctions;
        std::map<std::string, std::function<void(Gep::Entity, const nlohmann::json&)>> mLoadComponentFunctions;
    };
}

#include "SerializationSystem.inl"