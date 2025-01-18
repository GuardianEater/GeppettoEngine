/*****************************************************************//**
 * \file   ImGuiSystem.hpp
 * \brief  System that operates imgui
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

 // backend
#include <imgui.h>
#include <ISystem.hpp>

// client

namespace Client
{
    class ImGuiSystem : public Gep::ISystem
    {
    public:
        ImGuiSystem(Gep::EngineManager& em)
            : ISystem(em)
        {
        }
    };
}
