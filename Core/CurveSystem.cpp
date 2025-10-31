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

        if (mManager.IsState(Gep::EngineState::Play))
            UpdatePathFollowers(dt); // path followers are updated to the location on the curve
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
            const glm::vec3 uaPos = curve.spline.Evaluate(ua);
            const glm::vec3 umPos = curve.spline.Evaluate(um);
            const glm::vec3 ubPos = curve.spline.Evaluate(ub);
            const float A = glm::length(uaPos - umPos);
            const float B = glm::length(umPos - ubPos);
            const float C = glm::length(uaPos - ubPos);
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

    void CurveSystem::UpdatePathFollowers(float dt)
    {
        mManager.ForEachArchetype<Client::Transform, Client::PathFollowerComponent>(
        [&](Gep::Entity ent, Client::Transform& transform, Client::PathFollowerComponent& pfc)
        {
            Gep::Entity targetEntity = mManager.FindEntity(pfc.targetPathEntity);

            // if the entity doesnt exist do nothing. Also the entity must have curve/transform
            if (!mManager.EntityExists(targetEntity)) return;
            if (!mManager.HasComponent<Client::CurveComponent>(targetEntity)) return;
            if (!mManager.HasComponent<Client::Transform>(targetEntity)) return;

            CurveComponent& curve = mManager.GetComponent<CurveComponent>(targetEntity);

            // progess down the path
            pfc.distanceAlongPath += dt * pfc.speed;

            // clamp time / if looping is on loop
            if (pfc.distanceAlongPath > curve.arcLength)
            {
                if (pfc.looping)
                    pfc.distanceAlongPath = 0.0f;
                else
                    pfc.distanceAlongPath = curve.arcLength;
            }
            else if (pfc.distanceAlongPath < 0.0f)
            {
                if (pfc.looping)
                    pfc.distanceAlongPath = curve.arcLength;
                else
                    pfc.distanceAlongPath = 0.0f;
            }


            CurveComponent& path = mManager.GetComponent<Client::CurveComponent>(targetEntity);
            Transform& pathTransform = mManager.GetComponent<Client::Transform>(targetEntity);

            // if the current entity has an animation component
            if (mManager.HasComponent<Client::AnimationComponent>(ent))
            {
                AnimationComponent& animation = mManager.GetComponent<AnimationComponent>(ent);
            }
            
            glm::mat4 model = pathTransform.GetModelMatrix();
            transform.position = model * glm::vec4(EvaluateAtDistance(path, pfc.distanceAlongPath), 1.0f);
            glm::vec3 nextPoint = EvaluateAtDistance(path, pfc.distanceAlongPath + 1.0f);
            nextPoint = model * glm::vec4(nextPoint, 1.0f);

            glm::vec3 lookVector = nextPoint - transform.position;

            // Orient transform so its local forward (+Z) points toward the look vector.
            // Skip if the look vector is degenerate.
            if (glm::dot(lookVector, lookVector) > glm::epsilon<float>())
            {
                glm::vec3 forward = glm::normalize(lookVector);

                // Choose a world-up; avoid near-parallel up/forward which would cause a degenerate basis.
                glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
                if (std::abs(glm::dot(forward, worldUp)) > 0.9999f)
                    worldUp = glm::vec3(1.0f, 0.0f, 0.0f);

                // Build an orthonormal basis: right, up, forward (columns)
                glm::vec3 right = glm::normalize(glm::cross(worldUp, forward));
                glm::vec3 up = glm::cross(forward, right);

                glm::mat3 rotMat(right, up, forward); // columns -> local axes aligned with world-space basis
                glm::quat orientation = glm::quat_cast(rotMat);

                // Store Euler angles in degrees so existing Gep::rotation(rotation) (which expects degrees)
                // produces the correct model matrix in GetModelMatrix().
                transform.rotation = glm::degrees(glm::eulerAngles(orientation));
            }

        });
    }

    void CurveSystem::EvaluateCubicSplinePoints(CurveComponent& curve)
    {
        // evaluate uniformly for rendering
        for (uint32_t i = 0; i < curve.subdivisions; ++i)
        {
            // note: one less control point and add it after to be exatcly at the end
            const double t = (double)i / curve.subdivisions;
            curve.points.push_back(curve.spline.Evaluate(t));
        }
        // ensure the end point is exact
        curve.points.push_back(curve.controlPoints.back());
    }

    glm::vec3 CurveSystem::EvaluateAtDistance(const Client::CurveComponent& curve, double distance) const
    {
        // noop if there is nothing in the lookup table
        if (curve.lookUpTable.empty())
            return {};

        // clamp before
        if (distance <= 0.0)
            return curve.spline.Evaluate(0.0);

        // clamp after
        if (distance >= curve.arcLength)
            return curve.spline.Evaluate(1.0);

        // std::lower_bound is a binary search, finds the first item greater than the key
        auto it = std::lower_bound(curve.lookUpTable.begin(), curve.lookUpTable.end(), distance, 
        [](const auto& entry, double value) 
        {
            return entry.arcLength < value;
        });

        auto prev = std::prev(it);

        const auto& [t0, d0] = *prev;
        const auto& [t1, d1] = *it;

        // interpolate parameter t based on how far between the two distances we are
        double alpha = (distance - d0) / (d1 - d0);
        double t = t0 + alpha * (t1 - t0);

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

        ImGui::BeginGroup(); // group for drag drop

        ImGui::TextDisabled(uuidString.c_str());

        bool entityExists = mManager.EntityExists(targetEntity);

        // checks if the followed entity exists
        if (!entityExists)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not Following an entity");
            return;
        }

        bool hasPath = mManager.HasComponent<CurveComponent>(targetEntity);
        bool hasTransform = mManager.HasComponent<Transform>(targetEntity);

        // checks if the followed entity has the need components
        if (!hasPath || !hasTransform)
        {
            ImGui::Text("Following Entity:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), mManager.GetName(targetEntity).c_str());

            if (!hasPath)
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Missing Curve Component");
            if (!hasTransform)
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Missing Transform");

            return;
        }

        CurveComponent& curve = mManager.GetComponent<CurveComponent>(targetEntity);

        // conditions met display the entity and continue with the inspector items
        ImGui::Text("Following Entity:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), mManager.GetName(targetEntity).c_str());
        ImGui::EndGroup();

        mEditor.EntityDragDropTarget([&](Gep::Entity e)
        {
            event.component.targetPathEntity = mManager.GetUUID(e);
        });

        const double min = 0.0;
        const double max = curve.arcLength;
        ImGui::DragScalar("Distance Along Path", ImGuiDataType_Double, &event.component.distanceAlongPath, 0.5, &min, &max);
        ImGui::DragFloat("Speed Down Path", &event.component.speed);
        ImGui::Checkbox("Looping", &event.component.looping);

        bool hasAnimation = mManager.HasComponent<Transform>(event.entity);
        if (hasAnimation)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Animation Component Found");

        }
    }

}
