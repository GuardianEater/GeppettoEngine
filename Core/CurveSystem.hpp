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
    class OpenGLRenderer;
    class CubicSpline;
}

namespace Gep::Event
{
    template <typename ComponentType> struct ComponentEditorRender;
    template <typename ComponentType> struct ComponentAdded;
}

namespace Client
{
    class EditorResource;
    struct CurveComponent;
    struct PathFollowerComponent;
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
        void UpdateArcLengthTable(Client::CurveComponent& curve);
        void UpdatePathFollowers(float dt);
        void EvaluateCubicSplinePoints(Client::CurveComponent& curve);

        // takes a distance and returns a t value from 0-1
        double GetTFromDistance(const Client::CurveComponent& curve, double distance) const;

        double ParabolicEase(double t, double t1, double t2) const;
        double ParabolicEaseVelocity(double t, double t1, double t2) const;

        void OnCurveEditorRender(const Gep::Event::ComponentEditorRender<Client::CurveComponent>& cc);
        void OnCurveAdded(const Gep::Event::ComponentAdded<Client::CurveComponent>& event);

        void OnPathFollowerEditorRender(const Gep::Event::ComponentEditorRender<Client::PathFollowerComponent>& cc);

        Gep::OpenGLRenderer& mRenderer;
        Client::EditorResource& mEditor;
    };
}
