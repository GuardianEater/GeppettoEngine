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
            {
                outLocalPose[track.boneIndex] = mRenderer.Interpolate(track, time);
            }
        }
    }

    // takes a bone structure in local pose and outputs it as global pose
    static void CalculateGlobalPose(const Gep::Skeleton& skeleton, std::vector<Gep::VQS>& outGlobalPose)
    {
        for (uint16_t i = 1; i < skeleton.bones.size(); i++) // note skip the root bone
        {
            const Gep::Bone& bone = skeleton.bones[i];

            outGlobalPose[i] = outGlobalPose[bone.parentIndex] * outGlobalPose[i];
        }
    }

    static void DrawSkeleton(const Gep::Skeleton& skeleton, const glm::mat4& modelMatrix, const std::vector<Gep::VQS>& globalPose, Gep::LineGPUData& line)
    {
        for (int i = 0; i < skeleton.bones.size(); i++) 
        {
            int parent = skeleton.bones[i].parentIndex;
            if (parent != Gep::num_max<uint16_t>()) 
            {
                glm::vec4 a = glm::vec4(globalPose[parent].position, 1.0f);
                glm::vec4 b = glm::vec4(globalPose[i].position, 1.0f);

                a = modelMatrix * a;
                b = modelMatrix * b;

                line.points.push_back({a, b}); // however you render debug lines
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

            // progress the animation
            animationComponent.currentTime += dt * animationComponent.speed * animation.ticksPerSecond;

            // clamp time / if looping is on loop
            if (animationComponent.currentTime > animation.duration)
            {
                if (animationComponent.looping)
                    animationComponent.currentTime = 0.0f;
                else
                    animationComponent.currentTime = animation.duration;
            }
            else if (animationComponent.currentTime < 0.0f)
            {
                if (animationComponent.looping)
                    animationComponent.currentTime = animation.duration;
                else
                    animationComponent.currentTime = 0.0f;
            }

            std::vector<Gep::VQS> localPose(model.skeleton.bones.size());

            EvaluateAnimation(animation, animationComponent.currentTime, localPose);

            CalculateGlobalPose(model.skeleton, localPose);

            DrawSkeleton(model.skeleton, transform.GetModelMatrix(), localPose, line);
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
