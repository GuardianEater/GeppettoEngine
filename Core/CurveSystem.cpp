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
#include "ImGuiHelp.hpp"

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
        mManager.ForEachArchetype<Client::Transform, Client::CurveComponent>([&](Gep::Entity ent, const Client::Transform& transform, Client::CurveComponent& curveComponent)
        {
            if (curveComponent.dirty)
            {
                curveComponent.spline.SetControlPoints(curveComponent.controlPoints); // updates the control points of the spline
                curveComponent.points.clear();

                EvaluateCubicSplinePoints(curveComponent); // fills the points variable with points allong the line
                UpdateArcLengthTable(curveComponent);      // fills the arc length table

                curveComponent.dirty = false;
            }

            const glm::mat4 model = Gep::ToMat4(transform.world);
            const glm::mat4 normal = Gep::NormalFromModel(model);

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

            // adds a line segment for all evaluated points
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

            if (D > 0.01f)
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

            const glm::mat4 model = Gep::ToMat4(pathTransform.local);

            const double t0 = pfc.easeTimes.first;
            const double t1 = pfc.easeTimes.second;

            // progess down the path, only if playing
            if (mManager.IsState(Gep::EngineState::Play))
                pfc.linearDistance += dt * pfc.speedAdjust * pfc.pace;

            // clamp time, if looping is on: loop
            pfc.linearDistance = Gep::WrapOrClamp(pfc.linearDistance, 0.0, curve.arcLength, pfc.looping);

            // updated the eased distance
            const double percentageDownPath = pfc.linearDistance / curve.arcLength;
            double easedPercentage = ParabolicEase(percentageDownPath, t0, t1);
            pfc.distanceAlongPath = easedPercentage * curve.arcLength;

            // go a little into the future to see where we should look
            const double nextPos = Gep::WrapOrClamp(pfc.distanceAlongPath + 1.0, 0.0, curve.arcLength, pfc.looping);

            // get the current t value along the curve 
            const double thisT = GetTFromDistance(curve, pfc.distanceAlongPath);
            const double nextT = GetTFromDistance(curve, nextPos);

            // and get the position at it
            const glm::vec3 thisPoint = model * glm::vec4(curve.spline.Evaluate(thisT), 1.0f);
            const glm::vec3 nextPoint = model * glm::vec4(curve.spline.Evaluate(nextT), 1.0f);

            // if this entity has an animation set its animation speed
            if (mManager.HasComponent<Client::AnimationComponent>(ent))
            {
                AnimationComponent& animation = mManager.GetComponent<AnimationComponent>(ent);

                const double easedVelocity = ParabolicEaseVelocity(percentageDownPath, t0, t1);
                animation.speedModifier = easedVelocity * pfc.pace;
            }

            // get where the object should be looking
            const glm::vec3 lookVector = nextPoint - thisPoint;
            const glm::quat q = Gep::QuatFromLook(lookVector);

            // update this objects transform
            transform.local.position = thisPoint;

            // prevents edge case if looping is off. Makes objects look direction not snap to 0,0,0
            if (glm::dot(lookVector, lookVector) > 0.001) 
                transform.local.rotation = q;
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
        if (t1 > t2) std::swap(t1, t2);

        // make sure all t values are from 0 -> 1
        t = glm::clamp(t, 0.0, 1.0);
        t1 = glm::clamp(t1, 0.0 + glm::epsilon<double>(), 1.0); // clamp from zero to one, preventing division by zero
        t2 = glm::clamp(t2, 0.0, 1.0 - glm::epsilon<double>()); // clamp from zero to one, preventing division by zero

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

    double CurveSystem::ParabolicEaseVelocity(double t, double t1, double t2) const
    {
        // dont attemp ease if t1 is less than t2
        if (t1 > t2) std::swap(t1, t2);

        // make sure all t values are from 0 -> 1
        t = glm::clamp(t, 0.0, 1.0);
        t1 = glm::clamp(t1, 0.0 + glm::epsilon<double>(), 1.0); // clamp from zero to one, preventing division by zero
        t2 = glm::clamp(t2, 0.0, 1.0 - glm::epsilon<double>()); // clamp from zero to one, preventing division by zero

        // naming convention follows notes
        const double v0 = 2.0 / ((1.0 - t1) + t2);

        // section 1 if between 0 and t1
        if (0.0 <= t && t <= t1)
        {
            float v1 = t / t1;
            return v1 * v0;
        }

        // section 2 if between t1 and t2
        if (t1 <= t && t <= t2)
        {
            float v2 = 1.0;
            return v2 * v0;
        }

        // section 3 if between t2 and 1
        if (t2 <= t && t <= 1.0)
        {
            float v3 = (1.0 - t) / (1.0 - t2);
            return v3 * v0;
        }

        return 0.0; // this shouldn't be possible. the last if check is for completeness
    }

    void CurveSystem::OnCurveEditorRender(const Gep::Event::ComponentEditorRender<CurveComponent>& event)
    {
        CurveComponent& cc = *event.components[0];
        Gep::Entity e = event.entities[0];

        ImGui::PushID(e);

        if (ImGui::InputScalar("Segments", ImGuiDataType_U64, &cc.subdivisions))
            cc.dirty = true;

        if (ImGui::TreeNode("Control Points"))
        {
            for (int i = 0; i < cc.controlPoints.size(); ++i)
            {
                const size_t id = reinterpret_cast<const size_t>(glm::value_ptr(cc.controlPoints[i])); // interpret pointer as a number

                ImGui::PushID(id);
                if (ImGui::DragFloat3("##", glm::value_ptr(cc.controlPoints[i])))
                    cc.dirty = true;

                ImGui::SameLine();
                if (ImGui::Button("X", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    cc.controlPoints.erase(cc.controlPoints.begin() + i);
                    cc.dirty = true;
                    --i;
                }
                ImGui::PopID();

            }

            if (ImGui::Button("+", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                cc.controlPoints.emplace_back(0.0f);
                cc.dirty = true;
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
        bool valid = mEditor.DrawEntityDragDropTarget<Client::CurveComponent, Client::Transform>(mManager, "Target Path Entity", event.components,
            [](Client::PathFollowerComponent* pfc) -> Gep::UUID& { return pfc->targetPathEntity; }
        );

        // if the needed checks failed dont continue with the ui
        if (!valid) return;

        Gep::ImGui::MultiDragFloat("Animation Offset", event.components,
            [](Client::PathFollowerComponent* pfc) -> float& { return pfc->speedAdjust; }
        );

        Gep::ImGui::MultiDragFloat("Pace", event.components,
            [](Client::PathFollowerComponent* pfc) -> float& { return pfc->pace; }
        );

        Gep::ImGui::MultiCheckbox("Looping", event.components,
            [](Client::PathFollowerComponent* pfc) -> bool& { return pfc->looping; }
        );

        bool tChanged = false;

        tChanged |= Gep::ImGui::MultiSliderScalar("t0", event.components, 0.0f, 1.0f,
            [](Client::PathFollowerComponent* pfc) -> float& { return pfc->easeTimes.first; }
        );

        tChanged |= Gep::ImGui::MultiSliderScalar("t1", event.components, 0.0f, 1.0f,
            [](Client::PathFollowerComponent* pfc) -> float& { return pfc->easeTimes.second; }
        );

        if (tChanged)
        {
            for (Client::PathFollowerComponent* pfc : event.components)
            {
                // clamp t0 and t1 to be valid
                pfc->easeTimes.first = std::clamp(pfc->easeTimes.first, 0.0f, 1.0f);
                pfc->easeTimes.second = std::clamp(pfc->easeTimes.second, 0.0f, 1.0f);
            }
        }

        if (event.components.size() != 1)
            return; // only support single selection for the rest of the ui

        Client::PathFollowerComponent& pfc = *event.components[0];
        Gep::Entity e = event.entities[0];
        Gep::Entity targetEntity = mManager.FindEntity(pfc.targetPathEntity);
        CurveComponent& curve = mManager.GetComponent<CurveComponent>(targetEntity);

        // --- Parabolic easing visualization ---
        constexpr int numSamples = 200;
        static float positionValues[numSamples];
        static float velocityValues[numSamples];
        const float t0 = pfc.easeTimes.first;
        const float t1 = pfc.easeTimes.second;
        const float t = pfc.linearDistance / curve.arcLength;

        // evaluate positions
        for (int i = 0; i < numSamples; ++i)
        {
            float t = i / float(numSamples - 1); // [0, 1]
            positionValues[i] = ParabolicEase(t, t0, t1);
        }

        // evaluate velocity
        for (int i = 0; i < numSamples; ++i)
        {
            float t = i / float(numSamples - 1); // [0, 1]
            velocityValues[i] = ParabolicEaseVelocity(t, t0, t1);
        }

        // Plot
        ImGui::Text("Position-Time");
        ImGui::PlotLines("##pos", positionValues, numSamples, 0, nullptr, 0.0f, 2.0f, ImVec2(0, 100));

        // --- Add dotted lines ---
        // Get draw list and plot region
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 plotMin = ImGui::GetItemRectMin();
        ImVec2 plotMax = ImGui::GetItemRectMax();

        float plotWidth = plotMax.x - plotMin.x;
        float plotHeight = plotMax.y - plotMin.y;

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
        drawDottedLine(t, IM_COL32(255, 255, 255, 255)); // current time line

        ImGui::Text("Velocity-Time");
        ImGui::PlotLines("##vel", velocityValues, numSamples, 0, nullptr, 0.0f, 2.0f, ImVec2(0, 100));

        plotMin = ImGui::GetItemRectMin();
        plotMax = ImGui::GetItemRectMax();

        plotWidth = plotMax.x - plotMin.x;
        plotHeight = plotMax.y - plotMin.y;

        drawDottedLine(t0, IM_COL32(255, 100, 100, 255)); // red line
        drawDottedLine(t1, IM_COL32(100, 255, 100, 255)); // green line
        drawDottedLine(t, IM_COL32(255, 255, 255, 255)); // current time line

        const double min = 0.0;
        const double max = curve.arcLength;

        // progress bar
        ImGui::Text("Linear Distance");
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1)); // Reduce vertical padding
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 5.0f); // Set grab size to 5px
        ImGui::SliderScalar("##linearDistance", ImGuiDataType_Double, &pfc.linearDistance, &min, &max, "");
        ImGui::PopStyleVar(2);

        // progress bar time in seconds
        ImGui::Text("%.2f / %.2f", pfc.linearDistance, curve.arcLength);


        bool hasAnimation = mManager.HasComponent<AnimationComponent>(e);
        if (hasAnimation)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Animation Component Found");

        }
    }

}
