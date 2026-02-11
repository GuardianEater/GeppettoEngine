/*********************************************************************
 * file:   CurveComponent.hpp
 * author: travis.gronvold (travis.gronvold@digipen.edu)
 * date:   October 20, 2025
 * Copyright © 2023 DigiPen (USA) Corporation. 
 * 
 * brief:  
 *********************************************************************/

#pragma once

#include "CubicSpline.hpp"

namespace Client
{
    // defines an entity as a path that path followers can use
    struct CurveComponent
    {
        std::vector<glm::vec3> controlPoints{ // defines the points controlled by the user
            {0.0f, 0.0f, 1.0f}, 
            {0.0f, 1.0f, 0.0f}, 
            {0.0f, 0.0f, -1.0f} 
        };
        size_t subdivisions = 100;            // the amount of line segments to render when drawn
        glm::vec3 color = {1.0f, 1.0f, 1.0f}; // the color of the line when drawn

        std::vector<glm::vec3> points{}; // points cache, stores precomputed points used for rendering
        bool dirty = true;               // whether or not any values have changed, must be set manually.

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

        Gep::CubicSpline spline;             // object for computing and evaluating the spline at a normalized t value
        std::vector<TableEntry> lookUpTable; // used to find paramter values from a given arclength
        double arcLength = 0.0f;             // the accumulated/total arclength of the curve
    };
}
