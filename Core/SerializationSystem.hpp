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

        void OnAssetBrowserItemClicked(const Gep::Event::AssetBrowserItemClicked& event);

        void Initialize() override;
        void Exit() override;
    };
}

#include "SerializationSystem.inl"