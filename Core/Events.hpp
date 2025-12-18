/*****************************************************************//**
 * \file   EntityDestroyed.hpp
 * \brief  A bunch of predefined events
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <nlohmann/json.hpp>

namespace Gep
{
    template<typename EventType>
    using EventFunction = std::function<void(EventType)>;

    namespace Event
    {
        struct IEvent {};

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

        // signalled after the entity is attached
        struct EntityAttached
        {
            Entity child;
            Entity parent;
        };

        // signalled prior to the entity being detached
        struct EntityDetached
        {
            Entity child;
            Entity parent;
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

        struct FileDropped
        {
            std::vector<std::filesystem::path> droppedFiles; // the path to the dropped files
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

        struct AssetBrowserItemClicked
        {
            std::filesystem::path path;
            std::string extension;
        };

        // replaced OnImGuiRender in components, gives the entity that the component belongs to and the component
        template <typename ComponentType>
        struct ComponentEditorRender
        {
            const std::span<Entity> entities;
            const std::span<ComponentType*> components;
        };

        // event is called after the component has been serialized but before it has been commited to disk.
        // useful for making last second advanced modifications to the json data based on component data.
        template <typename ComponentType>
        struct ComponentSerializing
        {
            const ComponentType& component; // contains the component being serialized
            nlohmann::json& componentJson; // contains the automatically reflected component data

            Entity entity; // parent entity
        };

        // event is called immediately after the component has been deserialized
        // useful for making calls, such as loading from a different file based on a filepath that the component may contain
        template <typename ComponentType>
        struct ComponentDeserializing
        {
            ComponentType& component; // contains automatically reflected data from the componentJson
            const nlohmann::json& componentJson; // contains the original json data as read in from the file

            Entity entity; // parent
        };

        struct EngineStateChanged
        {
            EngineState previousState;
            EngineState newState;
        };
    }
}
