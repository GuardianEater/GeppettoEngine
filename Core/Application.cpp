/*****************************************************************//**
 * \file   Application.cpp
 * \brief  
 * 
 * \author Travis Gronvold
 * \date   January 2025
 *********************************************************************/

#include "Application.hpp"

namespace Gep
{
    std::function<void(const Event::KeyPressed&)> Application::mKeyCallbackFunc;

    void Application::Initialize_GLFW()
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

    void Application::Initialize_ImGui()
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
            style.FramePadding.y = 8;
            style.ItemSpacing.y = 4;
            style.FrameBorderSize = 3;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
            style.Colors[ImGuiCol_TitleBgActive]  = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            style.Colors[ImGuiCol_Header]         = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            style.Colors[ImGuiCol_HeaderHovered]  = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            style.Colors[ImGuiCol_HeaderActive]   = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
            style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            style.Colors[ImGuiCol_FrameBgActive]  = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(mPrimaryWindow, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    void Application::FrameStart_GLFW()
    {
        glfwPollEvents();
    }

    void Application::FrameStart_ImGui()
    {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Application::FrameEnd_ImGui()
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

    void Application::FrameEnd_GLFW()
    {
        glfwSwapBuffers(mPrimaryWindow);
    }

    void Application::End_GLFW()
    {
        glfwDestroyWindow(mPrimaryWindow);
        glfwTerminate();
    }

    void Application::End_ImGui()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    bool Application::Running() const
    {
        return !glfwWindowShouldClose(mPrimaryWindow);
    }

    void Application::GLFW_ErrorCallback(int error, const char* description)
    {
        std::cerr << "GLFW Error (" << error << ") " << description << std::endl;
    }

    void Application::GLFW_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        mKeyCallbackFunc({ key, action, scancode, mods });
    }

    void Application::GLFW_MouseCallback(GLFWwindow* window, int button, int action, int mods)
    {
        std::cout << "Button: [" << button << "]" << std::endl
            << "Action: [" << action << "]" << std::endl
            << "Mods:   [" << mods << "]" << std::endl << std::endl;
    }

    void Application::GLFW_WindowResizeCallback(GLFWwindow* window, int width, int height)
    {
        std::cout << "Window Size: [" << width << ", " << height << "]" << std::endl;
    }

    void Application::GLFW_WindowPositionCallback(GLFWwindow* window, int x, int y)
    {
        std::cout << "Window Position: [" << x << ", " << y << "]" << std::endl;
    }
}
