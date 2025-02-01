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
        struct IEvent
        {
            virtual ~IEvent() = 0;
        };

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
    }
}
