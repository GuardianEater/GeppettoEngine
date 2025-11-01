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
            Transform& pathTransform = mManager.GetComponent<Client::Transform>(targetEntity);

            // ensure the correct amount of t values
            //pfc.easeTs.resize(curve.controlPoints.size());

            // progess down the path on if playing
            if (mManager.IsState(Gep::EngineState::Play))
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

            // if the current entity has an animation component
            if (mManager.HasComponent<Client::AnimationComponent>(ent))
            {
                AnimationComponent& animation = mManager.GetComponent<AnimationComponent>(ent);
            }
            
            glm::mat4 model = pathTransform.GetModelMatrix();

            double t = 0.0;
            t = GetTFromDistance(curve, pfc.distanceAlongPath);
            transform.position = model * glm::vec4(curve.spline.Evaluate(t), 1.0f);

            t = GetTFromDistance(curve, pfc.distanceAlongPath + 1.0);
            const glm::vec3 nextPoint = model * glm::vec4(curve.spline.Evaluate(t), 1.0f);

            const glm::vec3 lookVector = nextPoint - transform.position;

            // orient transform so its points toward the look vector.
            // skip if the look vector is degenerate.
            if (glm::dot(lookVector, lookVector) > glm::epsilon<float>())
            {
                glm::vec3 forward = glm::normalize(lookVector);

                // world-up: avoid near-parallel up/forward
                glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
                if (std::abs(glm::dot(forward, worldUp)) > 1.0f - glm::epsilon<float>())
                    worldUp = glm::vec3(1.0f, 0.0f, 0.0f);

                // orthonormal basis: right, up, forward
                glm::vec3 right = glm::normalize(glm::cross(worldUp, forward));
                glm::vec3 up = glm::cross(forward, right);

                glm::mat3 rotMat(right, up, forward); // columns -> local axes aligned with world-space basis
                glm::quat orientation = glm::quat_cast(rotMat);

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

    double CurveSystem::GetTFromDistance(const Client::CurveComponent& curve, double distance) const
    {
        // noop if there is nothing in the lookup table
        if (curve.lookUpTable.empty())
            return 0.0;

        // clamp before
        if (distance <= 0.0)
            return 0.0;

        // clamp after
        if (distance >= curve.arcLength)
            return 1.0;

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

        return t;
    }

    double CurveSystem::ParabolicEase(double t, double t1, double t2) const
    {
        // dont attemp ease if t1 is less than t2
        if (t1 < t2) std::swap(t1, t2);

        // make sure all t values are from 0 -> 1
        //t = glm::clamp(t, 0.0f, 1.0f);
        //t1 = glm::clamp(t1, 0.0f, 1.0f);
        //t2= glm::clamp(t2, 0.0f, 1.0f);

        // naming convention follows notes

        const double v0 = 2.0 / ((1.0 - t1) + t2);

        // section 1 if between 0 and t1
        if (0.0 <= t && t <= t1)
        {
            double s1 = (v0 / (2.0 * t1)) * t * t;

            return s1;
        }

        // section 2 if between t1 and t2
        if (t1 <= t && t <= t2)
        {
            double s2 = v0 * (t - (t1 / 2.0));

            return s2;
        }

        // section 3 if between t2 and 1
        if (t2 <= t && t <= 1.0)
        {
            double s3_1 = (v0 * (t - t2)) / (2.0 * (1.0 - t2));
            double s3_2 = ((2.0 - t) - t2);
            double s3_3 = v0 * (t2 - (t1 / 2.0));

            double s3 = s3_1 * s3_2 + s3_3;

            return s3;
        }

        return 0.0; // this shouldn't be possible. the last if check is for completeness
    }

    float CurveSystem::ParabolicEaseVelocity(float t, float t1, float t2) const
    {
        // dont attemp ease if t1 is less than t2
        if (t1 < t2) std::swap(t1, t2);

        // make sure all t values are from 0 -> 1
        t = glm::clamp(t, 0.0f, 1.0f);
        t1 = glm::clamp(t1, 0.0f, 1.0f);
        t2 = glm::clamp(t2, 0.0f, 1.0f);

        // naming convention follows notes

        // section 1 if between 0 and t1
        if (0.0f <= t && t <= t1)
        {
            float v1 = t / t1;

            return v1;
        }

        // section 2 if between t1 and t2
        if (t1 <= t && t <= t2)
        {
            float v2 = 1.0f;

            return v2;
        }

        // section 3 if between t2 and 1
        if (t2 <= t && t <= 1.0f)
        {
            float v3 = (1.0f - t) / (1.0f - t2);

            return v3;
        }

        return 0.0f; // this shouldn't be possible. the last if check is for completeness
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
        const Gep::Entity targetEntity = mManager.FindEntity(event.component.targetPathEntity);
        const std::string uuidString = event.component.targetPathEntity.ToString();

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
        
        if (ImGui::TreeNode("Control Points"))
        {
            //for (auto& ts : event.component.easeTs)
            //{
            //    ImGui::DragFloat2("##", &ts.t0);
            //}

            ImGui::Spacing();
            ImGui::TreePop();
        }

        // --- Parabolic easing visualization ---
        if (ImGui::TreeNode("Parabolic Easing Preview"))
        {
            constexpr int numSamples = 200;
            static float values[numSamples];
            static float t0 = 0.25f;
            static float t1 = 0.75f;

            ImGui::DragFloat("t0", &t0, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("t1", &t1, 0.01f, 0.0f, 1.0f);

            // Compute values
            for (int i = 0; i < numSamples; ++i)
            {
                float t = i / float(numSamples - 1); // [0, 1]
                values[i] = ParabolicEase(t, t0, t1);
            }

            // Plot
            ImVec2 graphSize = ImVec2(0, 100);
            ImGui::PlotLines("##8", values, numSamples, 0, nullptr, 0.0f, 1.0f, graphSize);

            // --- Add dotted lines ---
            // Get draw list and plot region
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 plotMin = ImGui::GetItemRectMin();
            ImVec2 plotMax = ImGui::GetItemRectMax();

            const float plotWidth = plotMax.x - plotMin.x;
            const float plotHeight = plotMax.y - plotMin.y;

            // Helper lambda to draw a dotted vertical line
            auto drawDottedLine = [&](float t, ImU32 color)
            {
                float x = plotMin.x + t * plotWidth;
                float yTop = plotMin.y;
                float yBottom = plotMax.y;

                const float segmentLength = 4.0f;
                for (float y = yTop; y < yBottom; y += segmentLength * 2.0f)
                {
                    drawList->AddLine(ImVec2(x, y), ImVec2(x, y + segmentLength), color, 1.0f);
                }
            };

            // Draw t0 and t1 markers
            drawDottedLine(t0, IM_COL32(255, 100, 100, 255)); // red line
            drawDottedLine(t1, IM_COL32(100, 255, 100, 255)); // green line

            ImGui::TreePop();
        }
        // --- end visualization ---


        bool hasAnimation = mManager.HasComponent<Transform>(event.entity);
        if (hasAnimation)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Animation Component Found");

        }
    }

}
