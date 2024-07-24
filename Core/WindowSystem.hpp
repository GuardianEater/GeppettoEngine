/*****************************************************************//**
 * \file   WindowSystem.hpp
 * \brief  Used to handle the window
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <assert.h>

#include <System.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>

namespace Client
{
	void _glfw_error_callback(int error, const char* description)
	{
		fprintf(stderr, "GLFW Error %d: %s\n", error, description);
	}

	class WindowSystem : public Gep::ISystem
	{
	public:
		WindowSystem(Gep::EngineManager& em)
			: ISystem(em)
            , mPrimaryWindow(nullptr)
            , mIO(nullptr)
		{
            glfwSetErrorCallback(_glfw_error_callback);
            assert(glfwInit() && "Failed To Create Window");

            const char* glsl_version = "#version 430";
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            // Create window with graphics context
            mPrimaryWindow = glfwCreateWindow(1280, 720, "Engine", nullptr, nullptr);
            if (!mPrimaryWindow)
            {
                glfwTerminate();
                assert("Failed To Create Window");
            }


            glfwMakeContextCurrent(mPrimaryWindow);
            glfwSwapInterval(1); // Enable vsync

            glewExperimental = GL_TRUE; // Ensure GLEW uses modern techniques for managing OpenGL functionality
            if (glewInit() != GLEW_OK)
            {
                glfwDestroyWindow(mPrimaryWindow);
                glfwTerminate();
                assert("Failed To Create Window");
            }

            // Check OpenGL version
            const GLubyte* renderer = glGetString(GL_RENDERER);
            const GLubyte* version = glGetString(GL_VERSION);
            std::cout << "Renderer: " << renderer << std::endl;
            std::cout << "OpenGL version supported: " << version << std::endl;


            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            mIO = &ImGui::GetIO();
            mIO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            mIO->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
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
            }

            // Setup Platform/Renderer backends
            ImGui_ImplGlfw_InitForOpenGL(mPrimaryWindow, true);
            ImGui_ImplOpenGL3_Init(glsl_version);
		}

        ~WindowSystem()
        {
            glfwTerminate();
        }

		void Init() override
		{
			
		}

        void Update(double dt) override
        {
            if (glfwWindowShouldClose(mPrimaryWindow)) mManager.ExitEngine();

            static bool show_demo_window = true;
            static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(mPrimaryWindow, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
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

            glfwSwapBuffers(mPrimaryWindow);
        }

    private:
        GLFWwindow* mPrimaryWindow;
        ImGuiIO* mIO;
	};
}
