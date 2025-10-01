/*****************************************************************//**
 * \file   AnimationSystem.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   September 2025
 *********************************************************************/

#include "pch.hpp"


// gep
#include "EngineManager.hpp"
#include "Mesh.hpp"

// client
#include "AnimationSystem.hpp"
#include "EditorResource.hpp"

// component
#include "AnimationComponent.hpp"
#include "ModelComponent.hpp"
#include "Transform.hpp"

#include "Events.hpp"

// resource
#include "OpenGLRenderer.hpp"

namespace Client
{
    AnimationSystem::AnimationSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mRenderer(em.GetResource<Gep::OpenGLRenderer>())
    {

    }

    void AnimationSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<AnimationComponent>>(this, &AnimationSystem::OnAnimationEditorRender);
    }

    void AnimationSystem::EvaluateAnimation(const Gep::Animation& animation, float time, std::vector<Gep::VQS>& outLocalPose)
    {
        for (const auto& track : animation.tracks)
        {
            if (track.boneIndex < outLocalPose.size())
                mRenderer.Interpolate(outLocalPose[track.boneIndex], track, time);
        }
    }

    static void CalculateGlobalPose(const Gep::Skeleton& skeleton,
        const std::vector<Gep::VQS>& localPose,
        std::vector<Gep::VQS>& outGlobalPose)
    {
        for (size_t i = 0; i < skeleton.bones.size(); ++i)
        {
            uint16_t parent = skeleton.bones[i].parentIndex;

            if (parent == Gep::num_max<uint16_t>()) // root bone
                outGlobalPose[i] = localPose[i];
            else
                outGlobalPose[i] = outGlobalPose[parent] * localPose[i];
        }
    }

    static void DrawSkeleton(const Gep::Skeleton& skeleton, const glm::mat4& modelMatrix, const std::vector<Gep::VQS>& globalPose, Gep::LineGPUData& line)
    {
        // 4. Draw lines for debug skeleton
        for (size_t i = 0; i < skeleton.bones.size(); ++i)
        {
            const auto& bone = skeleton.bones[i];
            if (bone.parentIndex != Gep::num_max<uint16_t>())
            {
                glm::vec3 p1 = globalPose[i].position;
                glm::vec3 p2 = globalPose[bone.parentIndex].position;

                // Transform skeleton into entity/world space
                p1 = glm::vec3(modelMatrix * glm::vec4(p1, 1));
                p2 = glm::vec3(modelMatrix * glm::vec4(p2, 1));

                line.points.push_back({ p1, p2 });
            }
        }
    }


    void AnimationSystem::Update(float dt)
    {
        Gep::LineGPUData line;
        line.color = { 1.0f, 0.0f, 0.0f };
        mManager.ForEachArchetype<AnimationComponent, ModelComponent, Transform>([&](Gep::Entity entity, AnimationComponent& animationComponent, const ModelComponent& modelComponent, const Transform& transform)
        {
            if (!mRenderer.IsAnimationLoaded(animationComponent.name)) 
                return; // return is continue in for_each loop

            const Gep::Model& model = mRenderer.GetModel(modelComponent.name);

            if (model.skeleton.bones.empty()) // do not operate on a skeleton with no bones
                return;

            const Gep::Animation& animation = mRenderer.GetAnimation(animationComponent.name);

            animationComponent.currentTime += dt * animationComponent.speed * animation.ticksPerSecond;
            if (animationComponent.currentTime > animation.duration)
                animationComponent.currentTime = fmod(animationComponent.currentTime, animation.duration);

            // 2. Evaluate animation at current time -> local pose
            std::vector<Gep::VQS> localPose(model.skeleton.bones.size());
            EvaluateAnimation(animation, animationComponent.currentTime, localPose);

            // 3. Convert local pose to global transforms
            std::vector<Gep::VQS> globalPose(model.skeleton.bones.size());
            CalculateGlobalPose(model.skeleton, localPose, globalPose);

            DrawSkeleton(model.skeleton, transform.GetModelMatrix(), globalPose, line);
        });

        mRenderer.AddLine(line);
    }

    void AnimationSystem::OnAnimationEditorRender(const Gep::Event::ComponentEditorRender<AnimationComponent>& event)
    {
        AnimationComponent& animationComponent = event.component;

        Client::EditorResource& er = mManager.GetResource<Client::EditorResource>();
        std::vector<std::string> loadedAnimations = mRenderer.GetLoadedAnimations();

        // drop down for selecting a model
        bool meshesOpen = ImGui::BeginCombo("Animations", animationComponent.name.c_str());

        const std::vector<std::string>& allowedExtensions = mRenderer.GetSupportedModelFormats();

        er.AssetBrowserDropTarget(allowedExtensions, [&](const std::filesystem::path& droppedPath)
        {
            if (!mRenderer.IsMeshLoaded(droppedPath.string()))
            {
                mRenderer.AddModelFromFile(droppedPath.string());
            }
        });

        if (meshesOpen)
        {
            for (const std::string& animationName : loadedAnimations)
            {
                bool isSelected = (animationName == animationComponent.name);
                if (ImGui::Selectable(animationName.c_str(), isSelected))
                {
                    animationComponent.name = animationName;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (!mRenderer.IsAnimationLoaded(animationComponent.name))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No Animation Loaded");
            return;
        }

        const Gep::Animation& animation = mRenderer.GetAnimation(animationComponent.name);

        // basic attributes
        ImGui::DragFloat("Speed", &animationComponent.speed, 0.001f, 0.0f, 1.0f);
        ImGui::Checkbox("Looping", &animationComponent.looping);

        // progress bar
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1)); // Reduce vertical padding
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 5.0f); // Set grab size to 5px
        ImGui::SliderFloat("##Time", &animationComponent.currentTime, 0.0f, animation.duration, "");
        ImGui::PopStyleVar(2);

        // progress bar time in seconds
        ImGui::Text("%.2f / %.2f", animationComponent.currentTime / animation.ticksPerSecond, animation.duration / animation.ticksPerSecond);
    }
}
