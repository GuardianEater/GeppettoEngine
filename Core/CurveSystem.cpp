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
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Client::PathFollowerComponent>>(this, &CurveSystem::OnPathFollowerEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentSerializing<Client::CurveComponent>>([](const Gep::Event::ComponentSerializing<Client::CurveComponent>& cc)
        {
            cc.componentJson.erase("dirty"); // do not save the dirty variable
        });
    }

    void CurveSystem::Update(float dt)
    {
        UpdateFunctionLine();
    }


    void CurveSystem::UpdateFunctionLine()
    {
        Gep::LineGPUData line;
        line.color = { 0.0f, 0.0f ,1.0f };
        mManager.ForEachArchetype<Client::Transform, Client::CurveComponent>([&](Gep::Entity ent, Client::Transform& transform, Client::CurveComponent& curveComponent)
        {
            if (curveComponent.dirty)
            {
                curveComponent.points.clear();
                UpdateCubicSpline(curveComponent.controlPoints, curveComponent.subdivisions, curveComponent.points);
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

        mManager.ForEachArchetype<Client::PathFollowerComponent>([&](Gep::Entity ent, Client::PathFollowerComponent& pfc)
        {
            Gep::Entity targetEntity = mManager.FindEntity(pfc.targetPathEntity);

            // if the entity doesnt exist do nothing
            if (!mManager.EntityExists(targetEntity)) return;
        });
    }

    void CurveSystem::UpdateCubicSpline(const std::vector<glm::vec3>& controlPoints, const size_t resolution, std::vector<glm::vec3>& points)
    {
        const uint32_t n = static_cast<uint32_t>(controlPoints.size());
        if (n < 2) return;

        Gep::CubicSpline spline;
        
        spline.SetControlPoints(controlPoints);

        // evaluate uniformly for rendering
        for (uint32_t i = 0; i < resolution; ++i)
        {
            // note: one less control point and add it after to be exatcly at the end
            const float t = static_cast<float>(controlPoints.size() - 1) * i / resolution;
            points.push_back(spline.Evaluate(t));
        }
        // ensure the end point is exact
        points.push_back(controlPoints.back());

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

    void CurveSystem::OnPathFollowerEditorRender(const Gep::Event::ComponentEditorRender<Client::PathFollowerComponent>& event)
    {
        Gep::Entity targetEntity = mManager.FindEntity(event.component.targetPathEntity);

        std::string displayString = mManager.EntityExists(targetEntity) ? mManager.GetName(targetEntity) : "Not Following an entity";
        ImGui::Text(displayString.c_str());

        mEditor.EntityDragDropTarget([&](Gep::Entity e)
        {
            event.component.targetPathEntity = mManager.GetUUID(e);
        });
    }

}
