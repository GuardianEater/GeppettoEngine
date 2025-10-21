/*********************************************************************
 * file:   CurveSystem.hpp
 * author: travis.gronvold (travis.gronvold@digipen.edu)
 * date:   October 20, 2025
 * Copyright © 2023 DigiPen (USA) Corporation. 
 * 
 * brief:  uses the curve component to compute curves
 *********************************************************************/

#pragma once

#include "ISystem.hpp"

namespace Gep
{
    class EngineManager;
    struct CurveComponent;
    class OpenGLRenderer;
}

namespace Client
{
    class CurveSystem : public Gep::ISystem
    {
    public:
        CurveSystem(Gep::EngineManager& em);
        
        void Initialize() override;
        void Update(float dt) override;

    private:
        void UpdateFunctionLine();
        void UpdateCubicSpline(const std::vector<glm::vec3>& controlPoints, const size_t resolution, std::vector<glm::vec3>& points);

        Gep::OpenGLRenderer& mRenderer;
    };
}
