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

// component
#include "CurveComponent.hpp"
#include "Transform.hpp"

// resource
#include "OpenGLRenderer.hpp"

// external
#include <Eigen/Dense>

namespace Client
{
    static float TruncatedPower(float t, float c, float d)
    {
        if (t < c) return 0;

        return pow(t - c, d);
    }

    CurveSystem::CurveSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mRenderer(em.GetResource<Gep::OpenGLRenderer>())
    {

    }

    void CurveSystem::Initialize()
    {

    }

    void CurveSystem::Update(float dt)
    {
        UpdateFunctionLine();
    }


    void CurveSystem::UpdateFunctionLine()
    {
        static std::vector<glm::vec3> points; // storing this is a buffer for less allocations
        mManager.ForEachArchetype<Client::Transform, Client::CurveComponent>([&](Gep::Entity ent, Client::Transform& transform, Client::CurveComponent& curveComponent)
            {
                //if (!curveComponent.dirty) return; // skip this component if it has no changes

                Gep::LineGPUData line;
                line.color = curveComponent.color;

                points.clear();
                UpdateCubicSpline(curveComponent.controlPoints, curveComponent.subdivisions, points);

                for (size_t i = 0; i < points.size() - 1; ++i)
                {
                    const glm::vec3& p0 = points[i];
                    const glm::vec3& p1 = points[i + 1];

                    line.points.emplace_back(p0, p1);
                }

                mRenderer.AddLine(line);
            });
    }

    void CurveSystem::UpdateCubicSpline(const std::vector<glm::vec3>& controlPoints, const size_t resolution, std::vector<glm::vec3>& points)
    {
        const uint32_t n = static_cast<uint32_t>(controlPoints.size());
        if (n < 2) return;

        const uint32_t k = n - 1;
        const uint32_t dim = n + 2;

        Eigen::MatrixXf A(dim, dim);
        Eigen::VectorXf bx(dim), by(dim), bz(dim);

        A.setZero();
        bx.setZero();
        by.setZero();
        bz.setZero();

        // Data fitting rows (t = 0,1,...,k)
        for (uint32_t i = 0; i < n; ++i)
        {
            const float t = static_cast<float>(i);

            // polynomial part: [1, t, t^2, t^3]
            A(i, 0) = 1.0f;
            A(i, 1) = t;
            A(i, 2) = t * t;
            A(i, 3) = t * t * t;

            // truncated power terms: (t - j)^3_+ for j = 1..k-1
            for (uint32_t j = 1; j <= k - 1; ++j)
            {
                A(i, j + 3) = TruncatedPower(t, static_cast<float>(j), 3.0f);
            }

            bx[i] = controlPoints[i].x;
            by[i] = controlPoints[i].y;
            bz[i] = controlPoints[i].z;
        }

        // Natural spline boundary conditions: s''(0) = 0, s''(k) = 0
        const uint32_t r0 = n;
        const uint32_t r1 = n + 1;

        // s''(t) = 2c + 6dt + sum_j 6(t - j)_+ * alpha_j
        // At t = 0: 2c = 0
        A(r0, 2) = 2.0f;

        // At t = k: 2c + 6dk + sum_j 6(k - j) * alpha_j = 0
        A(r1, 3) = 6.0f * static_cast<float>(k);
        for (uint32_t j = 1; j <= k - 1; ++j)
        {
            A(r1, j + 3) = 6.0f * static_cast<float>(k - j);
        }

        // RHS for boundary rows (zeros) — previously missing/incorrect
        bx[r0] = by[r0] = bz[r0] = 0.0f;
        bx[r1] = by[r1] = bz[r1] = 0.0f;

        // Solve once and reuse the factorization
        const auto decomp = A.colPivHouseholderQr();
        Eigen::VectorXf coeffsX = decomp.solve(bx);
        Eigen::VectorXf coeffsY = decomp.solve(by);
        Eigen::VectorXf coeffsZ = decomp.solve(bz);

        // Evaluate
        for (uint32_t i = 0; i < resolution; ++i)
        {
            const float t = static_cast<float>(k) * (static_cast<float>(i) / static_cast<float>(resolution));

            glm::vec3 vt{
                coeffsX[0] + coeffsX[1] * t + coeffsX[2] * t * t + coeffsX[3] * t * t * t,
                coeffsY[0] + coeffsY[1] * t + coeffsY[2] * t * t + coeffsY[3] * t * t * t,
                coeffsZ[0] + coeffsZ[1] * t + coeffsZ[2] * t * t + coeffsZ[3] * t * t * t
            };

            for (size_t j = 1; j <= k - 1; ++j)
            {
                const float tp = TruncatedPower(t, static_cast<float>(j), 3.0f);
                vt.x += coeffsX[j + 3] * tp;
                vt.y += coeffsY[j + 3] * tp;
                vt.z += coeffsZ[j + 3] * tp;
            }

            points.push_back(vt);
        }

        // Ensure the end point is exact
        points.push_back(controlPoints.back());
    }

}
