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
    class CubicSpline
    {
    public:
        CubicSpline() = default;

        // needs to be called whener a value is changed
        void SetControlPoints(const std::vector<glm::vec3>& controlPoints);
        glm::vec3 Evaluate(double t) const;

    private:
        uint32_t controlPointCount = 0;

        Eigen::MatrixXd A;
        Eigen::VectorXd bx, by, bz;
        Eigen::VectorXd coeffsx, coeffsy, coeffsz;

    private:
        void ComputeAandB(const std::vector<glm::vec3>& controlPoints);
        void ComputeCoeffs();
    };
}

