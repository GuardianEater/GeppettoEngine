/*********************************************************************
 * file:   CurveComponent.hpp
 * author: travis.gronvold (travis.gronvold@digipen.edu)
 * date:   October 20, 2025
 * Copyright © 2023 DigiPen (USA) Corporation. 
 * 
 * brief:  
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>
#include "CubicSpline.h"

namespace Client
{

    // defines an entity as a path that path followers can use
    struct CurveComponent
    {
        // inputs for the curve //
        std::vector<glm::vec3> controlPoints{ // defines the points controlled by the user
            {0.0f, 0.0f, 1.0f}, 
            {0.0f, 1.0f, 0.0f}, 
            {0.0f, 0.0f, -1.0f} 
        };
        size_t subdivisions = 10; // the amount of line segments to render when drawn
        glm::vec3 color = {1.0f, 1.0f, 1.0f}; // the color of the line when drawn

        // cached values for optimization //
        std::vector<glm::vec3> points{}; // points cache, stores precomputed points for rendering
        bool dirty = true; // whether or not any values have changed, must be set manually.

        struct TableEntry
        {
            double parameterValue = 0.0f;
            double arcLength = 0.0f;
        };

        struct CurveSegment
        {
            double ua = 0.0f;
            double ub = 0.0f;
        };

        // used for evaluating arc length
        Gep::CubicSpline spline; // object for computing and evaluating the spline at a normalized t value
        std::vector<TableEntry> lookUpTable; // used to find paramter values from a given arclength
        //std::deque<CurveSegment> curveSegments; // stores all segments on the curve
        double arcLength = 0.0f; // the accumulated arclength of the curve
    };
}
