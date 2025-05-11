/*****************************************************************//**
 * \file   RigidBody.hpp
 * \brief  Component for storing movement data, velocity and the like
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <glm\glm.hpp>

namespace Client
{
    struct RigidBody
    {
        glm::vec3 velocity{};
        glm::vec3 acceleration{};
        glm::vec3 rotationalVelocity{};
        glm::vec3 rotationalAcceleration{};

        void OnScriptAccess(sol::usertype<RigidBody>& luaType)
        {
            luaType["velocity"] = &RigidBody::velocity;
            luaType["acceleration"] = &RigidBody::acceleration;
            luaType["rotationalVelocity"] = &RigidBody::rotationalVelocity;
            luaType["rotationalAcceleration"] = &RigidBody::rotationalAcceleration;
        }
    };
}
