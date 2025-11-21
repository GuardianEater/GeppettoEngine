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
#include "STLHelp.hpp"

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
    {}

    void AnimationSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<AnimationComponent>>(this, &AnimationSystem::OnAnimationEditorRender);
    }

    void AnimationSystem::EvaluateAnimation(const Gep::Animation& animation, float time, std::vector<Gep::VQS>& outLocalPose)
    {
        for (const auto& track : animation.tracks)
        {
            outLocalPose[track.boneIndex].position = mRenderer.InterpolatePosition(track, time);
            outLocalPose[track.boneIndex].rotation = mRenderer.InterpolateRotation(track, time);
            outLocalPose[track.boneIndex].scale = mRenderer.InterpolateScale(track, time);
        }
    }

    // takes a bone structure in local pose and outputs it as global pose
    static void CalculateGlobalPose(const Gep::Skeleton& skeleton, std::vector<Gep::VQS>& outGlobalPose)
    {
        for (uint32_t i = 1; i < skeleton.bones.size(); i++) // note skip the root bone
        {
            uint32_t parent = skeleton.bones[i].parentIndex;

            outGlobalPose[i] = outGlobalPose[parent] * outGlobalPose[i];
        }
    }

    void AnimationSystem::Update(float dt)
    {
        mManager.ForEachArchetype<AnimationComponent, ModelComponent, Transform>([&](Gep::Entity entity, AnimationComponent& animationComponent, ModelComponent& modelComponent, const Transform& transform)
        {
            if (!mRenderer.IsAnimationLoaded(animationComponent.name))
                return; // return is continue in for_each loop

            const Gep::Model& model = mRenderer.GetModel(modelComponent.name);

            if (model.skeleton.bones.empty()) // do not operate on a skeleton with no bones
                return;

            const Gep::Animation& animation = mRenderer.GetAnimation(animationComponent.name);

            // progress the animation
            if (mManager.IsState(Gep::EngineState::Play))
                animationComponent.currentTime += dt * animationComponent.speed * animationComponent.speedModifier *  animation.ticksPerSecond;

            // clamp time / if looping is on loop
            animationComponent.currentTime = Gep::WrapOrClamp(animationComponent.currentTime, 0.0f, animation.duration, animationComponent.looping);

            // I dont really understand why the clear has to be here but if I remove it there are strange anomalies sometimes.
            const uint32_t boneCount = static_cast<uint32_t>(model.skeleton.bones.size());
            modelComponent.pose.clear();
            modelComponent.pose.resize(boneCount);

            EvaluateAnimation(animation, animationComponent.currentTime, modelComponent.pose);

            CalculateGlobalPose(model.skeleton, modelComponent.pose);
        });
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
            if (!mRenderer.IsModelLoaded(droppedPath.string()))
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
        ImGui::SliderFloat("##AnimationTime", &animationComponent.currentTime, 0.0f, animation.duration, "");
        ImGui::PopStyleVar(2);

        // progress bar time in seconds
        ImGui::Text("%.2f / %.2f", animationComponent.currentTime / animation.ticksPerSecond, animation.duration / animation.ticksPerSecond);
    }
}
