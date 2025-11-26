// implementation for ik system

#include "pch.hpp"

// gep
#include "EngineManager.hpp"
#include "Events.hpp"
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

        mManager.ForEachArchetype<Client::IKTarget, Client::Transform>(
        [&](Gep::Entity e, Client::IKTarget& iktarget, Client::Transform& attractorTransform)
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

            if (!mRenderer.IsModelLoaded(modelComponent.name)) return;

            const Gep::Model& internalModel = mRenderer.GetModel(modelComponent.name);
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
        const Gep::Entity targetEntity = mManager.FindEntity(event.component.targetEntity);

        ImGui::BeginGroup(); // group for drag drop

        bool valid = false;

        // Check entity existence and required components, but avoid early returns.
        if (!mManager.EntityExists(targetEntity))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not Following an entity");
        }
        else
        {
            bool hasModel = mManager.HasComponent<RiggedModelComponent>(targetEntity);
            bool hasTransform = mManager.HasComponent<Transform>(targetEntity);

            ImGui::Text("Follower:");
            ImGui::SameLine();
            // display yellow if its missing a needed component
            ImGui::TextColored((hasModel && hasTransform) ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 0.0f, 1.0f), mManager.GetName(targetEntity).c_str());

            // display the missing components
            if (!hasModel)
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Missing Rigged Model Component");
            if (!hasTransform)
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Missing Transform");

            valid = hasModel && hasTransform;
        }

        ImGui::EndGroup();

        // drag drop for the entire group
        mEditor.EntityDragDropTarget([&](Gep::Entity e)
        {
            event.component.targetEntity = mManager.GetUUID(e);
        });

        // if the needed checks failed dont continue with the ui
        if (!valid) return;

        RiggedModelComponent& targetModel = mManager.GetComponent<RiggedModelComponent>(targetEntity);
        const Gep::Model& internalModel = mRenderer.GetModel(targetModel.name);

        if (targetModel.pose.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "The target has no bones!");
            return;
        }


        uint32_t startBoneIdx = event.component.startBone;
        if (ImGui::InputScalar("Start Bone (Anchor)", ImGuiDataType_U32, &startBoneIdx, nullptr, nullptr, nullptr, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            event.component.startBone = startBoneIdx;

            event.component.startBone = glm::clamp<uint32_t>(event.component.startBone, 0, targetModel.pose.size() - 1);
            if (event.component.startBone > event.component.endBone)
                event.component.startBone = event.component.endBone;
        }

        uint32_t endBone = event.component.endBone;
        if (ImGui::InputScalar("End Bone (Effector)", ImGuiDataType_U32, &endBone, nullptr, nullptr, nullptr, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            event.component.endBone = endBone;

            event.component.endBone = glm::clamp<uint32_t>(event.component.endBone, 0, targetModel.pose.size() - 1);
            if (event.component.startBone > event.component.endBone)
                event.component.startBone = event.component.endBone;
        }

    //    // drop down for selecting a model
    //    const Gep::Bone& startBone = internalModel.skeleton.bones[startBoneIdx];

    //    if (ImGui::BeginCombo("Start Bone", startBone.name.c_str()))
    //    {
    //        for (uint32_t currentBoneIdx = 0; currentBoneIdx < internalModel.skeleton.bones.size(); currentBoneIdx++)
    //        {
    //            const Gep::Bone& currentBone = internalModel.skeleton.bones.at(currentBoneIdx);

    //            bool isSelected = (currentBoneIdx == startBoneIdx);
    //            if (ImGui::Selectable(currentBone.name.c_str(), isSelected))
    //            {
    //                event.component.startBone = currentBoneIdx;
    //            }
    //            if (isSelected)
    //            {
    //                ImGui::SetItemDefaultFocus();
    //            }
    //        }
    //        ImGui::EndCombo();
    //    }
    }
}

