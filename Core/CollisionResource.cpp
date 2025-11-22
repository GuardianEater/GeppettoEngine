/*****************************************************************//**
 * \file   CollisionResource.cpp
 * \brief
 *
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   April 2025
 *********************************************************************/

#include "pch.hpp"

#include "CollisionResource.hpp"
#include "CollisionChecking.hpp"

#include "SphereCollider.hpp"
#include "CubeCollider.hpp"
#include "Transform.hpp"
#include "ModelComponent.hpp"
#include "MeshCollider.hpp"
#include "OpenGLRenderer.hpp"

namespace Client
{
    std::vector<Gep::Entity> CollisionResource::RayCast(Gep::EngineManager& em, const Gep::Ray& ray)
    {
        std::vector<Gep::Entity> hitEntities;

        std::vector<std::pair<float, Gep::Entity>> hits;

        // loop over all cube colliders
        em.ForEachArchetype<Client::Transform, Client::CubeCollider>([&](Gep::Entity entity, Client::Transform& transform, Client::CubeCollider& collider) 
        {
            float t;
            if (Gep::RayCube(ray, Gep::Cube{ transform.world.position, transform.world.scale * 0.5f, transform.world.rotation }, t))
            {
                if (t > 0.0f) // only accept objects infront of the ray
                    hits.emplace_back(t, entity);
            }
        });

        // loop over all sphere colliders
        em.ForEachArchetype<Client::Transform, Client::SphereCollider>([&](Gep::Entity entity, Client::Transform& transform, Client::SphereCollider& collider)
        {
            float t;
            if (Gep::RaySphere(ray, Gep::Sphere{ transform.world.position, std::max({transform.world.scale.x, transform.world.scale.y, transform.world.scale.z}) * 0.5f }, t))
            {
                if (t > 0.0f) // only accept objects infront of the ray
                    hits.emplace_back(t, entity);
            }
        });

        em.ForEachArchetype<Client::Transform, Client::ModelComponent, Client::MeshCollider>([&](Gep::Entity entity, Client::Transform& transform, Client::ModelComponent& model, Client::MeshCollider& collider)
        {
            Gep::OpenGLRenderer& renderer = em.GetResource<Gep::OpenGLRenderer>();
            const Gep::Model& mdl = renderer.GetModel(model.name);

            const glm::mat4 modelMtx = Gep::ToMat4(transform.world);

            float closestT = std::numeric_limits<float>::max();
            bool   anyHit = false;

            for (const Gep::Mesh& mesh : mdl.meshes)
            {
                // Coarse test: ray vs world-space AABB of this mesh
                // If your loader guarantees mesh.boundingBox is populated, this is fast.
                // Otherwise consider computing it here once.
                Gep::AABB worldAABB = TransformAABB(mesh.boundingBox, modelMtx);

                float taabb{};
                if (!Gep::RayAABB(ray, worldAABB, taabb))
                    continue;

                // Fine test: ray vs all triangles
                const auto& verts = mesh.vertices;
                const auto& idx = mesh.indices;

                // Guard: expect triangles
                if (idx.size() % 3 != 0) continue;

                for (size_t i = 0; i + 2 < idx.size(); i += 3)
                {
                    const glm::vec3 p0 = glm::vec3(modelMtx * glm::vec4(verts[idx[i + 0]].position, 1.0f));
                    const glm::vec3 p1 = glm::vec3(modelMtx * glm::vec4(verts[idx[i + 1]].position, 1.0f));
                    const glm::vec3 p2 = glm::vec3(modelMtx * glm::vec4(verts[idx[i + 2]].position, 1.0f));

                    float tTri{};
                    if (Gep::RayTriangle(ray, Gep::Triangle{ p0, p1, p2 }, tTri))
                    {
                        if (tTri > 0.0f && tTri < closestT)
                        {
                            closestT = tTri;
                            anyHit = true;
                        }
                    }
                }
            }

            if (anyHit)
                hits.emplace_back(closestT, entity);

        });


        // sort by time hit
        std::sort(hits.begin(), hits.end(), [](const auto& a, const auto& b) 
        {
            return a.first < b.first;
        });

        // return only the hit entities
        hitEntities.reserve(hits.size());
        for (const auto& hit : hits)
        {
            hitEntities.push_back(hit.second);
        }

        return hitEntities;
    }
}
