/*****************************************************************//**
 * \file   WindowSystem.cpp
 * \brief  Used to handle the window
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "pch.hpp"

#include "WindowSystem.hpp"


#include <numeric>
#include <algorithm>

namespace Client
{
    WindowSystem::WindowSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mPrimaryWindow(nullptr)
        , mIO(nullptr)
    {
        Initialize_GLFW();
        Initialize_ImGui();
    }

    void WindowSystem::Initialize()
    {
    }

    void WindowSystem::FrameStart()
    {
        FrameStart_GLFW();
        FrameStart_ImGui();
    }

    void WindowSystem::FrameEnd()
    {
        FrameEnd_ImGui();
        FrameEnd_GLFW();
    }

    void WindowSystem::Exit()
    {
        End_ImGui();
        End_GLFW();
    }

    void WindowSystem::Initialize_ImGui()
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
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(mPrimaryWindow, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    void WindowSystem::FrameStart_ImGui()
    {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
    }

    void WindowSystem::FrameEnd_ImGui()
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

    void WindowSystem::End_ImGui()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void WindowSystem::DrawUtilitiesWindow(float dt)
    {
        ImGui::Begin("Utilities");

        DrawFpsLog<60>(dt);
        DrawFrameTimes<100>(dt * 1000);

        static bool show_demo_window = false;
        ImGui::Checkbox("Demo Window", &show_demo_window);
        if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

        static bool show_style_window = false;
        ImGui::Checkbox("Style Window", &show_style_window);
        if (show_style_window)
        {
            ImGui::Begin("Style Editor");
            ImGui::ShowStyleEditor();
            ImGui::End();
        }

        static bool show_metrics_window = false;
        ImGui::Checkbox("Metrics Window", &show_metrics_window);
        if (show_metrics_window) ImGui::ShowMetricsWindow();

        ImGui::End();
    }

    template <size_t Size>
    void WindowSystem::DrawFrameTimes(float dt)
    {
        static std::array<float, Size> frameTimes;
        static float frameTimeSum = 0.0f;
        static size_t frameIndex = 0;

        float frameTime = dt;
        frameTimeSum -= frameTimes[frameIndex];
        frameTimes[frameIndex] = frameTime;
        frameTimeSum += frameTime;

        frameIndex = (frameIndex + 1) % frameTimes.size();

        ImGui::PlotHistogram("Frames Times", frameTimes.data(), frameTimes.size(), frameIndex, nullptr, 0, 50, ImVec2(0, 80));
    }

    template <size_t Size>
    void WindowSystem::DrawFpsLog(float dt)
    {
        static std::array<float, Size> frameCounts;
        static float timePassed = 0.0f;
        static float frames = 0;
        static size_t currentIndex = 0;
        static bool isDataLogFull = false;

        frames += 1.0f;
        timePassed += dt;

        if (timePassed > 1.0f)
        {
            frameCounts[currentIndex] = frames;
            currentIndex = (currentIndex + 1) % frameCounts.size();
            timePassed = 0;
            frames = 0;
        }

        ImGui::PlotHistogram("FPS Log", frameCounts.data(), frameCounts.size(), currentIndex, nullptr, 0, FLT_MAX, ImVec2(0, 80));

        static int averageTimeFrame = 10;
        ImGui::SliderInt("Average FPS\nTime Frame", &averageTimeFrame, 1, Size);

        if (currentIndex == frameCounts.size() - 1) isDataLogFull = true;

        if (!isDataLogFull && currentIndex < averageTimeFrame)
        {
            ImGui::Text("Average FPS: Calculating...");
            ImGui::Text("Minimum FPS: Calculating...");
        }
        else
        {
            if (averageTimeFrame > currentIndex)
            {
                size_t difference = averageTimeFrame - currentIndex;
                const float tail = std::accumulate(frameCounts.end() - difference, frameCounts.end(), 0.0);
                const float head = std::accumulate(frameCounts.begin(), frameCounts.begin() + currentIndex, 0.0);
                const float average = (tail + head) / averageTimeFrame;
                ImGui::Text("Average FPS: %.3f", average);

                const float smallestElementTail = *std::min_element(frameCounts.end() - difference, frameCounts.end());
                const float smallestElementHead = *std::min_element(frameCounts.begin(), frameCounts.begin() + currentIndex);
                const float smallestElement = (smallestElementTail < smallestElementHead) ? smallestElementTail : smallestElementHead;
                ImGui::Text("Minimum FPS: %.3f", smallestElement);
            }
            else
            {
                const float average = std::accumulate(frameCounts.begin() + currentIndex - averageTimeFrame, frameCounts.begin() + currentIndex, 0.0) / averageTimeFrame;
                ImGui::Text("Average FPS: %.3f", average);

                const float minimum = *std::min_element(frameCounts.begin() + currentIndex - averageTimeFrame, frameCounts.begin() + currentIndex);
                ImGui::Text("Minimum FPS: %.3f", minimum);
            }
        }
    }
    void WindowSystem::Initialize_GLFW()
    {
        ///////////////////////////////////////////////////////////////////////////////////////
        /// GLFW setup ////////////////////////////////////////////////////////////////////////
        glfwSetErrorCallback(GLFW_ErrorCallback);

        if (glfwInit() != GLFW_TRUE)
        {
            Gep::Log::Critical("Failed To Create Window");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

        // Create window with graphics context
        mPrimaryWindow = glfwCreateWindow(720, 720, "Engine", nullptr, nullptr);
        if (!mPrimaryWindow)
        {
            glfwTerminate();
            Gep::Log::Critical("Failed To Create Window");
        }

        glfwMakeContextCurrent(mPrimaryWindow);
        glfwSwapInterval(0); // disable vsync
        glfwSetWindowUserPointer(mPrimaryWindow, this);

        // set event functions
        glfwSetKeyCallback(mPrimaryWindow, GLFW_KeyCallback);
        glfwSetMouseButtonCallback(mPrimaryWindow, GLFW_MouseButtonCallback);
        glfwSetCursorPosCallback(mPrimaryWindow, GLFW_MousePositionCallback);
        glfwSetWindowSizeCallback(mPrimaryWindow, GLFW_WindowResizeCallback);
        glfwSetWindowPosCallback(mPrimaryWindow, GLFW_WindowPositionCallback);

        glewExperimental = GL_TRUE; // Ensure GLEW uses modern techniques for managing OpenGL functionality
        if (glewInit() != GLEW_OK)
        {
            glfwDestroyWindow(mPrimaryWindow);
            glfwTerminate();
            Gep::Log::Critical("Failed To Create Window");
        }

        int display_w = 0;
        int display_h = 0;
        glfwGetFramebufferSize(mPrimaryWindow, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);


        ///////////////////////////////////////////////////////////////////////////////////////
        /// Check OpenGL version //////////////////////////////////////////////////////////////

        const GLubyte* renderer = glGetString(GL_RENDERER);
        const GLubyte* version = glGetString(GL_VERSION);
        Gep::Log::Info("Renderer: ", renderer);
        Gep::Log::Info("OpenGL version supported: ", version);
    }

    void WindowSystem::FrameStart_GLFW()
    {
        glfwPollEvents();
    }

    void WindowSystem::FrameEnd_GLFW()
    {
        if (glfwWindowShouldClose(mPrimaryWindow))
        {
            mManager.SignalEvent<Gep::Event::WindowClosing>({});
        }
        glfwSwapBuffers(mPrimaryWindow);
    }

    void WindowSystem::End_GLFW()
    {
        glfwDestroyWindow(mPrimaryWindow);
        glfwTerminate();
    }

    void WindowSystem::GLFW_ErrorCallback(int error, const char* description)
    {
        Gep::Log::Critical("GLFW Error [", error, "] ", description);
    }

    void WindowSystem::GLFW_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        WindowSystem* ws = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        if (ws) ws->mManager.SignalEvent<Gep::Event::KeyPressed>({ key, scancode, action, mods });
    }

    void WindowSystem::GLFW_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        WindowSystem* ws = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        if (ws) ws->mManager.SignalEvent<Gep::Event::MouseClicked>({ button, action, mods });
    }

    void WindowSystem::GLFW_MousePositionCallback(GLFWwindow* window, double x, double y)
    {
        static double prevX = 0, prevY = 0;
        WindowSystem* ws = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        if (ws) ws->mManager.SignalEvent<Gep::Event::MouseMoved>({ x, y, prevX, prevY});
        prevX = x;
        prevY = y;
    }

    void WindowSystem::GLFW_WindowResizeCallback(GLFWwindow* window, int width, int height)
    {
        WindowSystem* ws = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        if (ws) ws->mManager.SignalEvent<Gep::Event::WindowResize>({ width, height });
    }

    void WindowSystem::GLFW_WindowPositionCallback(GLFWwindow* window, int x, int y)
    {
        WindowSystem* ws = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        if (ws) ws->mManager.SignalEvent<Gep::Event::WindowMoved>({ x, y });
    }
}



