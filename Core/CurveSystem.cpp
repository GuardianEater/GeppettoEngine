/*********************************************************************
 * file:   CurveSystem.cpp
 * author: travis.gronvold (travis.gronvold@digipen.edu)
 * date:   October 20, 2025
 * Copyright © 2023 DigiPen (USA) Corporation. 
 * 
 * brief:  Implementation of curve system
*********************************************************************/


// need
#include "pch.hpp"
#include "CurveSystem.hpp"
#include "EngineManager.hpp"

// helper
#include "STLHelp.hpp"
#include "JsonHelp.hpp"
#include "CubicSpline.h"

// component
#include "CurveComponent.hpp"
#include "Transform.hpp"
#include "PathFollowerComponent.hpp"
#include "AnimationComponent.hpp"

// resource
#include "OpenGLRenderer.hpp"
#include "EditorResource.hpp"

// external
#include <vector>

namespace Client
{
    CurveSystem::CurveSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mRenderer(em.GetResource<Gep::OpenGLRenderer>())
        , mEditor(em.GetResource<Client::EditorResource>())
    {

    }

    void CurveSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Client::CurveComponent>>(this, &CurveSystem::OnCurveEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<Client::CurveComponent>>(this, &CurveSystem::OnCurveAdded);

        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Client::PathFollowerComponent>>(this, &CurveSystem::OnPathFollowerEditorRender);
    }

    void CurveSystem::Update(float dt)
    { 
        UpdateFunctionLine(); // completes all curve components
        UpdatePathFollowers(); // path followers are updated to the location on the curve
    }


    void CurveSystem::UpdateFunctionLine()
    {
        Gep::LineGPUData line;
        line.color = { 0.0f, 0.0f ,1.0f };
        mManager.ForEachArchetype<Client::Transform, Client::CurveComponent>([&](Gep::Entity ent, Client::Transform& transform, Client::CurveComponent& curveComponent)
        {
            if (curveComponent.dirty)
            {
                curveComponent.spline.SetControlPoints(curveComponent.controlPoints); // updates the control points of the spline
                curveComponent.points.clear();

                EvaluateCubicSplinePoints(curveComponent); // fills the points variable with points allong the line
                UpdateArcLengthTable(curveComponent); // fills the arc length table

                curveComponent.dirty = false;
            }

            const glm::mat4 model = transform.GetModelMatrix();
            const glm::mat4 normal = transform.GetNormalMatrix(model);

            // Place a sphere on each control point
            for (const glm::vec3& cp : curveComponent.controlPoints)
            {
                glm::mat4 translation(1.0f);
                translation[3] = glm::vec4(cp, 1.0f); // local-space translation

                glm::mat4 cpModel = model * translation;
                cpModel = glm::scale(cpModel, glm::vec3(2.0f, 2.0f, 2.0f));

                Gep::ObjectGPUData uniformsCP
                {
                    .modelMatrix = cpModel,
                    .normalMatrixCol0 = normal[0],
                    .normalMatrixCol1 = normal[1],
                    .normalMatrixCol2 = normal[2],
                };

                mRenderer.AddObject("PBR-Static", "Icosphere", uniformsCP);
            }

            for (size_t i = 0; i + 1 < curveComponent.points.size(); ++i)
            {
                const glm::vec3 p0 = glm::vec3(model * glm::vec4(curveComponent.points[i], 1.0f));
                const glm::vec3 p1 = glm::vec3(model * glm::vec4(curveComponent.points[i + 1], 1.0f));

                line.points.emplace_back(p0, p1);
            }
        });
        mRenderer.AddLine(line);
    }

    void CurveSystem::UpdateArcLengthTable(Client::CurveComponent& curve)
    {
        // sets up the lookup table
        curve.lookUpTable.clear();
        curve.lookUpTable.emplace_back(0.0f, 0.0f);
        curve.arcLength = 0.0f;

        // computes all of the initial t values in the curve segments
        std::stack<CurveComponent::CurveSegment> curveSegments;
        for (int i = curve.controlPoints.size() - 2; i >= 0; --i)
        {
            float ua = static_cast<float>(i)     / (curve.controlPoints.size() - 1);
            float ub = static_cast<float>(i + 1) / (curve.controlPoints.size() - 1);
            curveSegments.push(CurveComponent::CurveSegment{ ua, ub });
        }

        while (!curveSegments.empty())
        {
            const auto[ua, ub] = curveSegments.top();// note copy by value so the references dont go stale
            const float um = (ua + ub) / 2.0f;
            const float A = glm::length(curve.spline.Evaluate(ua) - curve.spline.Evaluate(um));
            const float B = glm::length(curve.spline.Evaluate(um) - curve.spline.Evaluate(ub));
            const float C = glm::length(curve.spline.Evaluate(ua) - curve.spline.Evaluate(ub));
            const float D = abs(A + B - C);

            if (D > glm::epsilon<float>())
            {
                curveSegments.pop();
                curveSegments.emplace(um, ub);
                curveSegments.emplace(ua, um);
            }
            else
            {
                curveSegments.pop();
                curve.arcLength += A;
                curve.lookUpTable.emplace_back(um, curve.arcLength);
                curve.arcLength += B;
                curve.lookUpTable.emplace_back(ub, curve.arcLength);
            }
        }
    }

    void CurveSystem::UpdatePathFollowers()
    {
        mManager.ForEachArchetype<Client::Transform, Client::PathFollowerComponent>(
        [&](Gep::Entity ent, Client::Transform& transform, Client::PathFollowerComponent& pfc)
        {
            Gep::Entity targetEntity = mManager.FindEntity(pfc.targetPathEntity);

            // if the entity doesnt exist do nothing. Also the entity must have curve/transform
            if (!mManager.EntityExists(targetEntity)) return;
            if (!mManager.HasComponent<Client::CurveComponent>(targetEntity)) return;
            if (!mManager.HasComponent<Client::Transform>(targetEntity)) return;

            CurveComponent& path = mManager.GetComponent<Client::CurveComponent>(targetEntity);
            Transform& pathTransform = mManager.GetComponent<Client::Transform>(targetEntity);

            // if the current entity has an animation component
            if (mManager.HasComponent<Client::AnimationComponent>(ent))
            {
                AnimationComponent& animation = mManager.GetComponent<AnimationComponent>(ent);
            }
            
            glm::mat4 model = pathTransform.GetModelMatrix();
            transform.position = model * glm::vec4(EvaluateAtDistance(path, pfc.distanceAlongPath), 1.0f);
        });
    }

    void CurveSystem::EvaluateCubicSplinePoints(CurveComponent& curve)
    {
        // evaluate uniformly for rendering
        for (uint32_t i = 0; i < curve.subdivisions; ++i)
        {
            // note: one less control point and add it after to be exatcly at the end
            const float t = (float)i / curve.subdivisions;
            curve.points.push_back(curve.spline.Evaluate(t));
        }
        // ensure the end point is exact
        curve.points.push_back(curve.controlPoints.back());
    }

    glm::vec3 CurveSystem::EvaluateAtDistance(const Client::CurveComponent& curve, float distance) const
    {
        // noop if there is nothing in the lookup table
        if (curve.lookUpTable.empty())
            return {};

        // clamp before
        if (distance <= 0.0f)
            return curve.spline.Evaluate(0.0f);

        // clamp after
        if (distance >= curve.arcLength)
            return curve.spline.Evaluate(1.0f);

        // std::lower_bound is a binary search, finds the first item greater than the key
        auto it = std::lower_bound(curve.lookUpTable.begin(), curve.lookUpTable.end(), distance, 
        [](const auto& entry, float value) 
        {
            return entry.arcLength < value;
        });

        auto prev = std::prev(it);

        const auto& [t0, d0] = *prev;
        const auto& [t1, d1] = *it;

        // interpolate parameter t based on how far between the two distances we are
        float alpha = (distance - d0) / (d1 - d0);
        float t = t0 + alpha * (t1 - t0);

        // evaluate the spline at this normalized parameter
        return curve.spline.Evaluate(t);
    }

    void CurveSystem::OnCurveEditorRender(const Gep::Event::ComponentEditorRender<CurveComponent>& cc)
    {
        ImGui::PushID(cc.entity);

        if (ImGui::InputScalar("Segments", ImGuiDataType_U64, &cc.component.subdivisions))
            cc.component.dirty = true;

        if (ImGui::TreeNode("Control Points"))
        {
            for (int i = 0; i < cc.component.controlPoints.size(); ++i)
            {
                const size_t id = reinterpret_cast<const size_t>(glm::value_ptr(cc.component.controlPoints[i])); // interpret pointer as a number

                ImGui::PushID(id);
                if (ImGui::DragFloat3("##", glm::value_ptr(cc.component.controlPoints[i])))
                    cc.component.dirty = true;

                ImGui::SameLine();
                if (ImGui::Button("X", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    cc.component.controlPoints.erase(cc.component.controlPoints.begin() + i);
                    cc.component.dirty = true;
                    --i;
                }
                ImGui::PopID();

            }

            if (ImGui::Button("+", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                cc.component.controlPoints.emplace_back(0.0f);
                cc.component.dirty = true;
            }

            ImGui::Spacing();
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void CurveSystem::OnCurveAdded(const Gep::Event::ComponentAdded<Client::CurveComponent>& event)
    {
        event.component.dirty = true;
    }

    void CurveSystem::OnPathFollowerEditorRender(const Gep::Event::ComponentEditorRender<Client::PathFollowerComponent>& event)
    {
        Gep::Entity targetEntity = mManager.FindEntity(event.component.targetPathEntity);
        std::string uuidString = event.component.targetPathEntity.ToString();

        ImGui::BeginGroup();
        if (mManager.EntityExists(targetEntity))
        {
            ImGui::Text("Following Entity:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), mManager.GetName(targetEntity).c_str());
            //ImGui::TextDisabled(uuidString.c_str());
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not Following an entity");
        }
        ImGui::EndGroup();

        mEditor.EntityDragDropTarget([&](Gep::Entity e)
        {
            event.component.targetPathEntity = mManager.GetUUID(e);
        });

        ImGui::DragFloat("Distance Along Path", &event.component.distanceAlongPath);
    }

}
