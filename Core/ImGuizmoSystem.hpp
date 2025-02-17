/*****************************************************************//**
 * \file   ImGuizmoSystem.hpp
 * \brief  gizmo system for the engine
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"

#include "ISystem.hpp"


namespace Client
{
    class ImGuizmoSystem : public Gep::ISystem
    {
    public:
        ImGuizmoSystem(Gep::EngineManager& em)
            : ISystem(em)
        {}

        void Initialize() override;
        void Exit() override;
        void Update(float dt) override;
    };
}
