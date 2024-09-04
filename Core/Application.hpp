/*****************************************************************//**
 * \file   Application.hpp
 * \brief  Sets up a window and pull events 
 * 
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <Events.hpp>

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
        {}

	public:
        ///////////////////////////////////////////////////////////////////////
        // Foundation Functions ///////////////////////////////////////////////

        // Should be called before Initialize_ImGui
		void Initialize_GLFW()
		{
            ///////////////////////////////////////////////////////////////////////////////////////
            /// GLFW setup ////////////////////////////////////////////////////////////////////////
            glfwSetErrorCallback(GLFW_ErrorCallback);

            bool glfwSuccess = glfwInit();
            assert(glfwSuccess && "Failed To Create Window");

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

            // Create window with graphics context
            mPrimaryWindow = glfwCreateWindow(720, 720, "Engine", nullptr, nullptr);
            if (!mPrimaryWindow)
            {
                glfwTerminate();
                assert("Failed To Create Window");
                throw std::runtime_error("Failed to create window");
            }

            glfwMakeContextCurrent(mPrimaryWindow);
            glfwSwapInterval(0); // disable vsync

            // set event functions
            glfwSetKeyCallback(mPrimaryWindow, GLFW_KeyCallback);
            //glfwSetMouseButtonCallback(mPrimaryWindow, GLFW_MouseCallback);
            //glfwSetWindowSizeCallback(mPrimaryWindow, GLFW_WindowResizeCallback);
            //glfwSetWindowPosCallback(mPrimaryWindow, GLFW_WindowPositionCallback);

            glewExperimental = GL_TRUE; // Ensure GLEW uses modern techniques for managing OpenGL functionality
            if (glewInit() != GLEW_OK)
            {
                glfwDestroyWindow(mPrimaryWindow);
                glfwTerminate();
                assert("Failed To Create Window");
            }

            int display_w = 0;
            int display_h = 0;
            glfwGetFramebufferSize(mPrimaryWindow, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);


            ///////////////////////////////////////////////////////////////////////////////////////
            /// Check OpenGL version //////////////////////////////////////////////////////////////

            const GLubyte* renderer = glGetString(GL_RENDERER);
            const GLubyte* version = glGetString(GL_VERSION);
            std::cout << "Renderer: " << renderer << std::endl;
            std::cout << "OpenGL version supported: " << version << std::endl;
		}

        // Should be called after Initialize_GLFW
        void Initialize_ImGui()
        {
            ///////////////////////////////////////////////////////////////////////////////////////
            /// ImGui setup ///////////////////////////////////////////////////////////////////////
            const char* glsl_version = "#version 430";

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            mIO = &ImGui::GetIO();
            mIO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            mIO->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
            mIO->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();

            // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
            ImGuiStyle& style = ImGui::GetStyle();
            if (mIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                style.WindowRounding = 0.0f;
                style.Colors[ImGuiCol_WindowBg].w = 1.0f;
                style.FramePadding.y = 8;
                style.ItemSpacing.y = 4;
                style.FrameBorderSize = 3;
            }

            // Setup Platform/Renderer backends
            ImGui_ImplGlfw_InitForOpenGL(mPrimaryWindow, true);
            ImGui_ImplOpenGL3_Init(glsl_version);
        }

        // Should be called before FrameStart_ImGui
        void FrameStart_GLFW()
        {
            glfwPollEvents();
        }

        // Should be called after FrameStart_GLFW
        void FrameStart_ImGui()
        {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        // Should be called before FrameEnd_GLFW
        void FrameEnd_ImGui()
        {
            // Rendering
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Update and Render additional Platform Windows
            // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
            //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
            if (mIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }
        }

        // Should be called after FrameEnd_ImGui
        void FrameEnd_GLFW()
        {
            glfwSwapBuffers(mPrimaryWindow);
        }

        // Should be called before End_ImGui()
        void End_GLFW()
        {
            glfwDestroyWindow(mPrimaryWindow);
            glfwTerminate();
        }

        // Should be called after End_GLFW
        void End_ImGui()
        {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }

        // whether or not the current application should be running
        bool Running() const
        {
            return !glfwWindowShouldClose(mPrimaryWindow);
        }



        ///////////////////////////////////////////////////////////////////////
        // Foundation Functions ///////////////////////////////////////////////


        template<typename Type, typename TypeMemberFunction>
        static void SetKeyCallback(Type& object, TypeMemberFunction function)
        {
            mKeyCallbackFunc = std::bind(function, &object, std::placeholders::_1);
        }

    private:
        static void GLFW_ErrorCallback(int error, const char* description)
        {
            std::cerr << "GLFW Error (" << error << ") " << description << std::endl;
        }

        static std::function<void(const Event::KeyPressed&)> mKeyCallbackFunc;
        static void GLFW_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            mKeyCallbackFunc({key, action, scancode, mods});
        }

        static void GLFW_MouseCallback(GLFWwindow* window, int button, int action, int mods)
        {
            std::cout << "Button: [" << button  << "]" << std::endl
                      << "Action: [" << action  << "]" << std::endl
                      << "Mods:   [" << mods    << "]" << std::endl << std::endl;
        }

        static void GLFW_WindowResizeCallback(GLFWwindow* window, int width, int height)
        {
            std::cout << "Window Size: [" << width << ", " << height << "]" << std::endl;
        }

        static void GLFW_WindowPositionCallback(GLFWwindow* window, int x, int y)
        {
            std::cout << "Window Position: [" << x << ", " << y << "]" << std::endl;
        }

	private:
        GLFWwindow* mPrimaryWindow;
        ImGuiIO* mIO;
    };

    std::function<void(const Event::KeyPressed&)> Application::mKeyCallbackFunc;
}
