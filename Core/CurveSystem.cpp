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
        const size_t n = controlPoints.size();
        if (n < 2) return; // dont do anything if there is 0 or 1 points

        const size_t k = n - 1;

        Eigen::MatrixXf A(n + 2, n + 2);
        Eigen::VectorXf bx(n + 2);
        Eigen::VectorXf by(n + 2);
        Eigen::VectorXf bz(n + 2);

        A.setZero();
        bx.setZero();
        bx.setZero();
        bx.setZero();
        
        for (int i = 0; i < n; i++)
        {
            // evaluates thes a's
            float t = i;
            A(i, 0) = 1;
            A(i, 1) = t;
            A(i, 2) = t * t;
            A(i, 3) = t * t * t;

            // evaluates the b's
            for (int j = 1; j <= k - 1; j++)
            {
                A(i, j + 3) = TruncatedPower(t, j, 3);
            }

            // evaluates the v's
            bx[i] = controlPoints[i].x;
            by[i] = controlPoints[i].y;
            bz[i] = controlPoints[i].z;
        }
        bx[n] = 0;
        by[n] = 0;
        by[n + 1] = 0;
        by[n + 1] = 0;

        // boundary conditions
        A(n, 0) = 0;
        A(n, 1) = 0;
        A(n, 2) = 2;
        A(n, 3) = 0;
        for (size_t j = 1; j <= k - 1; j++)
        {
            A(n, j + 3) = 0;
        }

        A(n + 1, 0) = 0;
        A(n + 1, 1) = 0;
        A(n + 1, 2) = 0;
        A(n + 1, 3) = 6 * k;
        for (size_t j = 1; j <= k - 1; j++)
        {
            double contribution = 6 * (k - j);
            A(n + 1, j + 3) = contribution;
        }

        // solve the linear system
        //Eigen::ColPivHouseholderQR<float> linSystem(A);

        Eigen::VectorXf coeffsX = A.colPivHouseholderQr().solve(bx);
        Eigen::VectorXf coeffsY = A.colPivHouseholderQr().solve(by);
        Eigen::VectorXf coeffsZ = A.colPivHouseholderQr().solve(bz);

        // evaluate the spline
        for (size_t i = 0; i < resolution; ++i)
        {
            float t = ((float)k / resolution) * i;

            // evaluates the a's at t
            glm::vec3 vt{
                coeffsX[0] + (coeffsX[1] * t) + (coeffsX[2] * t * t) + (coeffsX[3] * t * t * t),
                coeffsY[0] + (coeffsY[1] * t) + (coeffsY[2] * t * t) + (coeffsY[3] * t * t * t),
                coeffsZ[0] + (coeffsZ[1] * t) + (coeffsZ[2] * t * t) + (coeffsZ[3] * t * t * t)
            };

            // evaluates the b's at ts
            for (int j = 1; j <= k - 1; j++)
            {
                vt.x += coeffsX[j + 3] * TruncatedPower(t, j, 3);
                vt.y += coeffsY[j + 3] * TruncatedPower(t, j, 3);
                vt.z += coeffsZ[j + 3] * TruncatedPower(t, j, 3);
            }

            points.push_back(vt);
        }

        points.push_back(controlPoints.back());
    }

}
