/*****************************************************************//**
 * \file   IKSystem.cpp
 * \brief  system for handling inverse kinematics using cyclic coordinate descent (CCD)
 * 
 * \author 2018t
 * \date   February 2026
 *********************************************************************/

// pch
#include "pch.hpp"

// engine
#include "EngineManager.hpp"
#include "Events.hpp"

// rendering
#include "Model.hpp"

// this
#include "IKSystem.hpp"

// component
#include "Transform.hpp"
#include "IKTarget.hpp"
#include "ModelComponent.hpp"

// resource
#include "EditorResource.hpp"
#include "OpenGLRenderer.hpp"

// help
#include "ImGuiHelp.hpp"
#include "GLMHelp.hpp"

namespace Client
{
    IKSystem::IKSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mEditor(em.GetResource<Client::EditorResource>())
        , mRenderer(em.GetResource<Gep::OpenGLRenderer>())
    {

    }

    void IKSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Client::IKTarget>>(this, &IKSystem::OnIKTargetEditorRender);
    }

    void IKSystem::Update(float dt)
    {
        // CCD parameters
        constexpr uint32_t kMaxIterations = 50;
        constexpr float    epsilon = 0.001f;

        mManager.ForEachArchetype([&](Gep::Entity e, IKTarget& iktarget, Transform& attractorTransform)
        {
            const Gep::Entity skeletonEntity = mManager.FindEntity(iktarget.targetEntity);
            if (!mManager.EntityExists(skeletonEntity)) return;
            if (!mManager.HasComponent<Client::RiggedModelComponent>(skeletonEntity)) return;
            if (!mManager.HasComponent<Client::Transform>(skeletonEntity)) return;

            Client::Transform& skeletonTransform = mManager.GetComponent<Client::Transform>(skeletonEntity);
            Client::RiggedModelComponent& modelComponent = mManager.GetComponent<Client::RiggedModelComponent>(skeletonEntity);

            if (modelComponent.pose.empty()) return;

            // Validate bone indices
            if (iktarget.endBone >= modelComponent.pose.size()) return;
            if (iktarget.startBone >= modelComponent.pose.size()) return;

            const uint32_t anchorBoneIdx = iktarget.startBone;
            const uint32_t effectorBoneIdx = iktarget.endBone;
            if (anchorBoneIdx > effectorBoneIdx) 
                return; // the anchor should never appear after the child

            if (!mRenderer.IsMeshLoaded(modelComponent.name)) return;

            const Gep::Mesh& internalModel = mRenderer.GetMesh(modelComponent.meshID);
            const Gep::Skeleton& skeleton = internalModel.skeleton;

            if (skeleton.bones.size() != modelComponent.pose.size())
            {
                Gep::Log::Error("Skeleton bones and model pose size do not match");
                return;
            }

            // convert target position into model-space (same space as pose)
            glm::vec3 targetPos = Gep::Inverse(skeletonTransform.world) * attractorTransform.world.position;

            // the models pose is in global space, this is a copy in local space. static for working memory reducing the excess amounts of allocations
            static std::vector<Gep::VQS> localPose; 
            localPose.clear(); 
            localPose.resize(modelComponent.pose.size());
            {
                localPose[0] = modelComponent.pose[0];
                for (size_t i = 1; i < modelComponent.pose.size(); ++i)
                {
                    uint32_t parent = skeleton.bones[i].parentIndex;

                    localPose[i] = Gep::Inverse(modelComponent.pose[parent]) * modelComponent.pose[i];
                }
            }

            // this takes the current local pose and converts it back to global
            auto RecalculateGlobal = [&](std::vector<Gep::VQS>& outGlobal)
            {
                outGlobal[0] = localPose[0];
                for (uint32_t i = 1; i < skeleton.bones.size(); ++i)
                {
                    uint32_t parent = skeleton.bones[i].parentIndex;
                    outGlobal[i] = outGlobal[parent] * localPose[i];
                }
            };

            float previousDistance = std::numeric_limits<float>::max();

            for (uint32_t iteration = 0; iteration < kMaxIterations; ++iteration)
            {
                glm::vec3 effectorPos = modelComponent.pose[effectorBoneIdx].position;
                float distanceToTarget = glm::distance(effectorPos, targetPos);
                if (distanceToTarget <= epsilon) break;

                float improvement = previousDistance - distanceToTarget;
                if (improvement >= 0.0f && improvement < epsilon) break;
                previousDistance = distanceToTarget;

                uint32_t jointIdx = effectorBoneIdx;

                while (jointIdx != anchorBoneIdx) 
                {
                    jointIdx = skeleton.bones[jointIdx].parentIndex;
                        
                    glm::vec3 jointPos = modelComponent.pose[jointIdx].position;

                    glm::vec3 toEffector = modelComponent.pose[effectorBoneIdx].position - jointPos;
                    glm::vec3 toTarget = targetPos - jointPos;

                    toEffector = glm::normalize(toEffector);
                    toTarget   = glm::normalize(toTarget);

                    float d = glm::clamp(glm::dot(toEffector, toTarget), -1.0f, 1.0f);

                    // stable quaternion for rotating from toEffector to toTarget
                    glm::quat deltaRot{};
                    if (d >= 1.0f - 1e-3)
                    {
                        // already aligned
                        continue;
                    }
                    else if (d <= -1.0f + epsilon)
                    {
                        // 180 degree turn, use an perpendicular stable axis
                        glm::vec3 axis = glm::cross(glm::vec3(1, 0, 0), toEffector);
                        if (glm::dot(axis, axis) < epsilon)
                            axis = glm::cross(glm::vec3(0, 1, 0), toEffector);
                        axis = glm::normalize(axis);
                        deltaRot = glm::angleAxis(glm::pi<float>(), axis);
                    }
                    else
                    {
                        glm::vec3 c = glm::cross(toEffector, toTarget);
                        float s = std::sqrt((1.0f + d) * 2.0f);
                        float invs = 1.0f / s;
                        deltaRot = glm::quat(s * 0.5f, c.x * invs, c.y * invs, c.z * invs);
                        deltaRot = glm::normalize(deltaRot);
                    }

                    // apply rotation in joint local space
                    // convert global delta into local frame so don't accumulate drift
                    glm::quat parentGlobalRot = (jointIdx == 0)
                        ? glm::quat(1, 0, 0, 0)
                        : modelComponent.pose[skeleton.bones[jointIdx].parentIndex].rotation;

                    glm::quat localDelta = glm::inverse(parentGlobalRot) * deltaRot * parentGlobalRot;

                    glm::quat newLocalRot = glm::normalize(localDelta * localPose[jointIdx].rotation);

                    // add constraints

                    localPose[jointIdx].rotation = newLocalRot;

                    RecalculateGlobal(modelComponent.pose);
                }
            }
        });
    }

    void IKSystem::OnIKTargetEditorRender(const Gep::Event::ComponentEditorRender<Client::IKTarget>& event)
    {
        bool valid = mEditor.DrawEntityDragDropTarget<Client::RiggedModelComponent, Client::Transform>(mManager, "Target Entity", event.components,
            [&](Client::IKTarget* ikt) -> gtl::uuid& { return ikt->targetEntity; }
        );

        // if the needed checks failed dont continue with the ui
        if (!valid) return;
        
        bool targetModelsUniform = Gep::IsUniform(event.components, 
            [&](Client::IKTarget* ikt) -> const std::string& 
            {
                const Gep::Entity targetEntity = mManager.FindEntity(ikt->targetEntity);
                const RiggedModelComponent& targetModel = mManager.GetComponent<RiggedModelComponent>(targetEntity);
                return targetModel.name; 
            }
        );

        if (!targetModelsUniform)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Target Models are not the same");
            return;
        }

        // get first target model for bone count reference
        const Client::IKTarget& firstIKTarget = *event.components[0];
        const Gep::Entity firstTargetEntity = mManager.FindEntity(firstIKTarget.targetEntity);

        bool startChanged = Gep::ImGui::MultiInputScalarX("Start Bone (Anchor)", event.components, 
            [&](Client::IKTarget* ikt) -> uint32_t& { return ikt->startBone; }
        );

        if (startChanged)
        {
            for (Client::IKTarget* iktarget : event.components)
            {
                Gep::Entity targetEntity = mManager.FindEntity(iktarget->targetEntity);
                iktarget->startBone = glm::clamp<uint32_t>(iktarget->startBone, 0, mManager.GetComponent<RiggedModelComponent>(targetEntity).pose.size() - 1);
                if (iktarget->startBone > iktarget->endBone)
                    iktarget->startBone = iktarget->endBone;
            }
        }

        bool endChanged = Gep::ImGui::MultiInputScalarX("End Bone (Effector)", event.components,
            [&](Client::IKTarget* ikt) -> uint32_t& { return ikt->endBone; }
        );

        if (endChanged)
        {
            for (Client::IKTarget* iktarget : event.components)
            {
                Gep::Entity targetEntity = mManager.FindEntity(iktarget->targetEntity);
                iktarget->endBone = glm::clamp<uint32_t>(iktarget->endBone, 0, mManager.GetComponent<RiggedModelComponent>(targetEntity).pose.size() - 1);
                if (iktarget->startBone > iktarget->endBone)
                    iktarget->startBone = iktarget->endBone;
            }
        }
    }
}

