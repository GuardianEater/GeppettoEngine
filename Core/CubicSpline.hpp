/*****************************************************************//**
 * \file   CubicSpline.h
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   October 2025
 *********************************************************************/

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <Eigen/Core>
#include <Eigen/QR>

namespace Gep
{
    // a structure that represents a cubic spline, it can be evaluated at any t value between 0 and 1 to get a point on the curve.
    class CubicSpline
    {
    public:
        CubicSpline() = default;

        // make the cubic spline fit the given control points
        void SetControlPoints(const std::vector<glm::vec3>& controlPoints);

        // gives the point on the curve at the given t value.
        glm::vec3 Evaluate(double t) const;

    private:
        uint32_t controlPointCount = 0; // the amount of control points given in set control points.

        // these 3 are used to compute the point on the curve
        Eigen::MatrixXd A; // the coefficient matrix
        Eigen::VectorXd bx, by, bz; // the constant vectors
        Eigen::VectorXd coeffsx, coeffsy, coeffsz; // the coefficient vectors

    private:
        void ComputeAandB(const std::vector<glm::vec3>& controlPoints);
        void ComputeCoeffs();
    };
}

