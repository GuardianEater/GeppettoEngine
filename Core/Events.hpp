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

        struct EntityDestroyed
        { 
            Entity entity;
        };

        struct EntityCreated
        {
            Entity entity;
        };

        struct KeyPressed
        {
            int keycode;
            int action; // whether the given keycode was held released or pressed, or held
            int scancode;
            int modifier; // wether or not shift/ctrl was held down
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

        template <typename ComponentType>
        struct ComponentAdded
        {
            Entity entity; // the entity the component was added to
        };
    }
}
