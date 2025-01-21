/*****************************************************************//**
 * \file   Application.hpp
 * \brief  Sets up a window and pull events
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include "Events.hpp"
#include "Logger.hpp"

#include <glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


namespace Gep
{
    class Application
    {
    public:
        Application()
            : mPrimaryWindow(nullptr)
            , mIO(nullptr)
        {
        }

    public:
        ///////////////////////////////////////////////////////////////////////
        // Foundation Functions ///////////////////////////////////////////////

        // Should be called before Initialize_ImGui
        void Initialize_GLFW();

        // Should be called after Initialize_GLFW
        void Initialize_ImGui();

        // Should be called before FrameStart_ImGui
        void FrameStart_GLFW();

        // Should be called after FrameStart_GLFW
        void FrameStart_ImGui();

        // Should be called before FrameEnd_GLFW
        void FrameEnd_ImGui();

        // Should be called after FrameEnd_ImGui
        void FrameEnd_GLFW();

        // Should be called before End_ImGui()
        void End_GLFW();

        // Should be called after End_GLFW
        void End_ImGui();

        // whether or not the current application should be running
        bool Running() const;



        ///////////////////////////////////////////////////////////////////////
        // Foundation Functions ///////////////////////////////////////////////


        template<typename Type, typename TypeMemberFunction>
        static void SetKeyCallback(Type& object, TypeMemberFunction function)
        {
            mKeyCallbackFunc = std::bind(function, &object, std::placeholders::_1);
        }

    private:
        static void GLFW_ErrorCallback(int error, const char* description);

        static void GLFW_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

        static void GLFW_MouseCallback(GLFWwindow* window, int button, int action, int mods);

        static void GLFW_WindowResizeCallback(GLFWwindow* window, int width, int height);

        static void GLFW_WindowPositionCallback(GLFWwindow* window, int x, int y);

    private:
        static std::function<void(const Event::KeyPressed&)> mKeyCallbackFunc;
        GLFWwindow* mPrimaryWindow;
        ImGuiIO* mIO;
    };
}
