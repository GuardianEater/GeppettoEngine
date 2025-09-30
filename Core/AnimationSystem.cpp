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

// component
#include "AnimationComponent.hpp"
#include "ModelComponent.hpp"
#include "Transform.hpp"

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
    }

    void AnimationSystem::EvaluateAnimation(const Gep::Animation& animation, float time, std::vector<Gep::VQS>& outLocalPose)
    {
        for (size_t boneIndex = 0; boneIndex < animation.tracks.size(); ++boneIndex)
        {
            const auto& track = animation.tracks[boneIndex];
            mRenderer.Interpolate(outLocalPose[boneIndex], track, time); // lerp trans, slerp rot, lerp scale
        }
    }

    static void CalculateGlobalPose(const Gep::Skeleton& skeleton,
        const std::vector<Gep::VQS>& localPose,
        std::vector<Gep::VQS>& outGlobalPose)
    {
        for (size_t i = 0; i < skeleton.bones.size(); ++i)
        {
            uint16_t parent = skeleton.bones[i].parentIndex;

            if (parent == Gep::num_max<uint16_t>())
                continue;

            if (parent >= 0)
                outGlobalPose[i] = outGlobalPose[parent] * localPose[i];
            else
                outGlobalPose[i] = localPose[i]; // root bone
        }
    }


    void AnimationSystem::Update(float dt)
    {
        Gep::LineGPUData line;
        line.color = { 1.0f, 0.0f, 0.0f };
        mManager.ForEachArchetype<AnimationComponent, ModelComponent, Transform>([&](Gep::Entity entity, AnimationComponent& animationComponent, const ModelComponent& modelComponent, const Transform& transform)
        {
            const Gep::Model& model = mRenderer.GetModel(modelComponent.name);
            const Gep::Animation& animation = mRenderer.GetAnimation(animationComponent.name);

            animationComponent.currentTime += dt * animationComponent.speed;
            if (animationComponent.currentTime > animation.duration)
                animationComponent.currentTime = fmod(animationComponent.currentTime, animation.duration);

            // 2. Evaluate animation at current time -> local pose
            std::vector<Gep::VQS> localPose(model.skeleton.bones.size());
            EvaluateAnimation(animation, animationComponent.currentTime, localPose);

            // 3. Convert local pose to global transforms
            std::vector<Gep::VQS> globalPose(model.skeleton.bones.size());
            CalculateGlobalPose(model.skeleton, localPose, globalPose);

            // 4. Draw lines for debug skeleton
            for (size_t i = 0; i < model.skeleton.bones.size(); ++i)
            {
                const auto& bone = model.skeleton.bones[i];
                if (bone.parentIndex != Gep::num_max<uint16_t>())
                {
                    glm::vec3 p1 = globalPose[i].position;
                    glm::vec3 p2 = globalPose[bone.parentIndex].position;

                    // Transform skeleton into entity/world space
                    glm::mat4 worldMat = transform.GetModelMatrix();
                    p1 = glm::vec3(worldMat * glm::vec4(p1, 1));
                    p2 = glm::vec3(worldMat * glm::vec4(p2, 1));

                    line.points.push_back({ p1, p2 });
                }
            }
        });

        mRenderer.AddLine(line);
    }
}
