/*****************************************************************//**
 * \file   ScriptingSystem.hpp
 * \brief  allows adding scripts to entities
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <ISystem.hpp>
#include <Script.hpp>
#include "EngineManager.hpp"

namespace Client
{
    class ScriptingSystem : public Gep::ISystem
    {
    public:
        ScriptingSystem(Gep::EngineManager& em);

        void Initialize();
        void Update(float dt);
    };
}


