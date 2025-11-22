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
    // Helper: rebuild world-space bone chain from root up to targetBone (inclusive).
    // Returns end-effector world transform.
    static void RebuildBoneWorlds(
        const Gep::VQS& rootWorld,
        const std::vector<Gep::VQS>& pose,
        uint32_t targetBone,
        std::vector<Gep::VQS>& outWorlds)
    {
        outWorlds.resize(targetBone + 1);
        Gep::VQS current = rootWorld;
        for (uint32_t i = 0; i <= targetBone; ++i)
        {
            current = current * pose[i];
            outWorlds[i] = current;
        }
    }

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
        constexpr uint32_t kMaxIterations = 15;
        constexpr float kDistanceEpsilon = 0.001f; // acceptable distance to target
        constexpr float kDeltaEpsilon = 0.0001f;   // improvement threshold

        mManager.ForEachArchetype<Client::IKTarget, Client::Transform>(
        [&](Gep::Entity e, Client::IKTarget& iktarget, Client::Transform& transform)
        {
            const Gep::Entity targetEntity = mManager.FindEntity(iktarget.targetEntity);
            if (!mManager.EntityExists(targetEntity)) return;
            if (!mManager.HasComponent<Client::ModelComponent>(targetEntity)) return;
            if (!mManager.HasComponent<Client::Transform>(targetEntity)) return;

            Client::Transform& targetTransform = mManager.GetComponent<Client::Transform>(targetEntity);
            Client::ModelComponent& targetModel = mManager.GetComponent<Client::ModelComponent>(targetEntity);

            if (targetModel.pose.empty()) return;

            // Validate bone indices
            if (iktarget.endBone >= targetModel.pose.size()) return;
            if (iktarget.startBone >= targetModel.pose.size()) return;
            if (iktarget.startBone > iktarget.endBone) return; // startBone must be ancestor (lower index) of endBone

            const uint32_t effectorBoneIdx = iktarget.endBone;
            const uint32_t anchorBoneIdx = iktarget.startBone;
            const uint32_t maxIteration = 15;

            const glm::vec3 Pd = targetTransform.world.position; // destination position
            glm::vec3& Pc = targetModel.pose[effectorBoneIdx].position; // current position
            glm::vec3 Ppc = Pc; // position from the last iteration

            // (a) If the distance between Pd and Pc is small enough, exit
            if (glm::distance(Pd, Pc) < 0.001f)
                return;

            const Gep::Model& internalModel = mRenderer.GetModel(targetModel.name); // get the bone hierarchy


            for (uint32_t i = 0; i < maxIteration; i++)
            {
                // (b) For each joint jk in the chain
                for (uint32_t currentBoneIdx = effectorBoneIdx; currentBoneIdx == anchorBoneIdx; currentBoneIdx = internalModel.skeleton.bones[currentBoneIdx].parentIndex)
                {
                    const Gep::VQS jk = targetModel.pose[currentBoneIdx];

                    // Make two vectors: Vck from jk to Pc and Vdk from jk to Pd;
                    const glm::vec3 Vck = Pc - jk.position;
                    const glm::vec3 Vdk = Pd - jk.position;

                    // Compute the angle between the vectors by dot product
                    const float ak = acos((glm::dot(Vck, Vdk)) / (glm::dot(glm::normalize(Vck), glm::normalize(Vdk))));

                    // Compute the cross product of two vectors
                    const glm::vec3 Vk = glm::cross(Vck, Vdk);

                    // Rotate link lk at jk hierarchically around Vk by ak
                    if (glm::dot(Vk, Vk) > glm::epsilon<float>())
                    {
                        glm::vec3 axis = glm::normalize(Vk);
                        glm::quat deltaRot = glm::angleAxis(ak, axis);

                        targetModel.pose[currentBoneIdx].rotation = glm::normalize(deltaRot * targetModel.pose[currentBoneIdx].rotation);
                    }

                    // If distance between Pd and Pc is small enough, exit
                    if (glm::distance(Pd, Pc) < 0.001f)
                        return;
                }

                // (c) If the change of current end-effector locations from two consecutive iterations is small enough, quit(no solution).
                if (glm::distance(Ppc, Pc) < 0.001f)
                    return;

                Ppc = Pc;

                // (d) Otherwise, repeat from step (b).
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
            bool hasModel = mManager.HasComponent<ModelComponent>(targetEntity);
            bool hasTransform = mManager.HasComponent<Transform>(targetEntity);

            ImGui::Text("Follower:");
            ImGui::SameLine();
            // display yellow if its missing a needed component
            ImGui::TextColored((hasModel && hasTransform) ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 0.0f, 1.0f), mManager.GetName(targetEntity).c_str());

            // display the missing components
            if (!hasModel)
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Missing Model Component");
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

        ModelComponent& targetModel = mManager.GetComponent<ModelComponent>(targetEntity);

        if (targetModel.pose.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "The target has no bones!");
            return;
        }

        if (ImGui::InputScalar("Start Bone (Anchor)", ImGuiDataType_U32, &event.component.startBone))
        {
            event.component.startBone = glm::clamp<uint32_t>(event.component.startBone, 0, targetModel.pose.size() - 1);
            if (event.component.startBone > event.component.endBone)
                event.component.startBone = event.component.endBone;
        }
        if (ImGui::InputScalar("End Bone (Effector)", ImGuiDataType_U32, &event.component.endBone))
        {
            event.component.endBone = glm::clamp<uint32_t>(event.component.endBone, 0, targetModel.pose.size() - 1);
            if (event.component.startBone > event.component.endBone)
                event.component.startBone = event.component.endBone;
        }

        ImGui::Text("Chain Length: %u", event.component.endBone - event.component.startBone + 1);

    }
}

