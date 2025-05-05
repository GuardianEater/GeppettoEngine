/*****************************************************************//**
 * \file   EntityDestroyed.hpp
 * \brief  A bunch of predefined events
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

namespace Gep
{
    template<typename EventType>
    using EventFunction = std::function<void(EventType)>;

    namespace Event
    {
        struct IEvent{};

        // signalled immediately before an entity is about to be destroyed
        struct EntityDestroyed
        { 
            Entity entity;
        };

        // signalled immediately after an entity has been constructed
        struct EntityCreated
        {
            Entity entity;
        };

        struct KeyPressed
        {
            int keycode;
            int action; // whether the given keycode was held released or pressed, or held
            int scancode;
            int modifier; // whether or not shift/ctrl was held down
        };

        struct WindowResize
        {
            int width;
            int height;
        };

        struct WindowMoved
        {
            int x;
            int y;
        };

        struct WindowClosing
        {
        };

        struct MouseMoved
        {
            double x;
            double y;
            double prevX;
            double prevY;
        };

        struct MouseClicked
        {
            int button;
            int action;
            int modifier;
        };

        struct MouseScrolled
        {
            double xoffset;
            double yoffset;
        };

        struct CollisionEnter
        {
            Entity entity;
        };

        struct CollisionStay
        {
            Entity entity;
        };

        struct CollisionExit
        {
            Entity entity;
        };

        // signalled immediately after the component was added to the entity
        template <typename ComponentType>
        struct ComponentAdded
        {
            Entity entity; // the entity the component was added to
            ComponentType& component;
        };

        // signalled immediately before the component is going to be destroyed
        template <typename ComponentType>
        struct ComponentRemoved
        {
            Entity entity;
            ComponentType& component;
        };
    }
}
