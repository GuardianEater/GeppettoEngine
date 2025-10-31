/*****************************************************************//**
 * \file   CubicSpline.cpp
 * \brief  implementation for the cubic spline class
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   October 2025
 *********************************************************************/

#include "pch.hpp"

#include "CubicSpline.h"

static double TruncatedPower(double t, double c, double d)
{
    if (t < c) return 0;

    return pow(t - c, d);
}

namespace Gep
{
    // needs to be called whener a value is changed
    void CubicSpline::SetControlPoints(const std::vector<glm::vec3>& controlPoints)
    {
        controlPointCount = controlPoints.size(); //used by evaluate

        ComputeAandB(controlPoints);
        ComputeCoeffs();
    }

    glm::vec3 CubicSpline::Evaluate(double t) const
    {
        if (controlPointCount < 2) return glm::vec3(0.0f);

        t *= static_cast<double>(controlPointCount - 1); // normalize the t value

        glm::vec3 vt{
            coeffsx[0] + coeffsx[1] * t + coeffsx[2] * t * t + coeffsx[3] * t * t * t,
            coeffsy[0] + coeffsy[1] * t + coeffsy[2] * t * t + coeffsy[3] * t * t * t,
            coeffsz[0] + coeffsz[1] * t + coeffsz[2] * t * t + coeffsz[3] * t * t * t
        };

        for (uint32_t j = 1; j <= controlPointCount - 2; ++j)
        {
            const double tp = TruncatedPower(t, static_cast<double>(j), 3.0f);
            vt.x += coeffsx[j + 3] * tp;
            vt.y += coeffsy[j + 3] * tp;
            vt.z += coeffsz[j + 3] * tp;
        }

        return vt;
    }

    void CubicSpline::ComputeAandB(const std::vector<glm::vec3>& controlPoints)
    {
        const uint32_t n = static_cast<uint32_t>(controlPoints.size());
        if (n < 2) return; // potentially clear everything to default state

        const uint32_t k = n - 1;
        const uint32_t dim = n + 2;

        A.resize(dim, dim);
        bx.resize(dim);
        by.resize(dim);
        bz.resize(dim);

        A.setZero();
        bx.setZero();
        by.setZero();
        bz.setZero();

        // Data fitting rows (t = 0,1,...,k)
        for (uint32_t i = 0; i < n; ++i)
        {
            const double t = static_cast<double>(i);

            // polynomial part: [1, t, t^2, t^3]
            A(i, 0) = 1.0;
            A(i, 1) = t;
            A(i, 2) = t * t;
            A(i, 3) = t * t * t;

            // truncated power terms: (t - j)^3_+ for j = 1..k-1
            for (uint32_t j = 1; j <= k - 1; ++j)
            {
                A(i, j + 3) = TruncatedPower(t, static_cast<double>(j), 3.0);
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
        A(r0, 2) = 2.0;

        // At t = k: 2c + 6dk + sum_j 6(k - j) * alpha_j = 0
        A(r1, 3) = 6.0 * static_cast<double>(k);
        for (uint32_t j = 1; j <= k - 1; ++j)
        {
            A(r1, j + 3) = 6.0 * static_cast<double>(k - j);
        }

        // RHS for boundary rows (zeros) — previously missing/incorrect
        bx[r0] = by[r0] = bz[r0] = 0.0;
        bx[r1] = by[r1] = bz[r1] = 0.0;
    }

    void CubicSpline::ComputeCoeffs()
    {
        // Solve once and reuse the factorization
        const auto decomp = A.colPivHouseholderQr();
        coeffsx = decomp.solve(bx);
        coeffsy = decomp.solve(by);
        coeffsz = decomp.solve(bz);
    }
}
