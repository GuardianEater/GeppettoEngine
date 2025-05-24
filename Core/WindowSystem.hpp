/*****************************************************************//**
 * \file   WindowSystem.hpp
 * \brief  Used to handle the window
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

 // core
#include <Core.hpp>
#include <ISystem.hpp>
#include <EngineManager.hpp>

// external
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// client
#include <Transform.hpp>
#include <Material.hpp>
#include <RigidBody.hpp>
#include <Script.hpp>

namespace Client
{
    class WindowSystem : public Gep::ISystem
    {
    public:
        WindowSystem(Gep::EngineManager& em);

        void Initialize() override;
        void FrameStart() override;
        void FrameEnd() override;
        void Exit() override;

    private:
        void Initialize_ImGui();
        void FrameStart_ImGui();
        void FrameEnd_ImGui();
        void End_ImGui();

        void Initialize_GLFW();
        void FrameStart_GLFW();
        void FrameEnd_GLFW();
        void End_GLFW();

        static void GLFW_ErrorCallback(int error, const char* description);
        static void GLFW_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void GLFW_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void GLFW_MousePositionCallback(GLFWwindow* window, double x, double y);
        static void GLFW_WindowResizeCallback(GLFWwindow* window, int width, int height);
        static void GLFW_WindowPositionCallback(GLFWwindow* window, int x, int y);
        static void GLFW_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void GLFW_DropCallback(GLFWwindow* window, int count, const char** paths);

    private:
        GLFWwindow* mPrimaryWindow;
        ImGuiIO* mIO;
        ImFont* mFont;

        void DrawUtilitiesWindow(float dt);

        template <size_t Size>
        void DrawFrameTimes(float dt);

        template <size_t Size>
        void DrawFpsLog(float dt);
    };
}



