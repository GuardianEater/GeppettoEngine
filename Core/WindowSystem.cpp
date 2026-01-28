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

#include <ImGuizmo.h>

namespace Client
{
    WindowSystem::WindowSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mPrimaryWindow(nullptr)
        , mIO(nullptr)
        , mFont(nullptr)
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
        const char* glsl_version = "#version 460";



        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        mIO = &ImGui::GetIO();
        mIO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        mIO->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
        mIO->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
        mIO->ConfigWindowsMoveFromTitleBarOnly = true;
        mFont = mIO->Fonts->AddFontFromFileTTF("assets\\fonts\\RobotoMono-Regular.ttf", 16.0f);

        ImFontConfig fontConfig;
        fontConfig.OversampleH = 3;
        fontConfig.OversampleV = 3;
        fontConfig.RasterizerDensity = 10;


        ImFont* defaultFont = mIO->Fonts->AddFontDefault(&fontConfig);

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        ImGuizmo::AllowAxisFlip(false);
        ImGuizmo::SetGizmoSizeClipSpace(0.2f);
        ImGuizmo::Style& gizmoStyle = ImGuizmo::GetStyle();
        gizmoStyle.TranslationLineThickness = 4.0f;
        gizmoStyle.TranslationLineArrowSize = 8.0f;
        gizmoStyle.CenterCircleSize = 5.0f;


        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (mIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImVec4 color = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            ImVec4 colorHovered = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
            ImVec4 colorActive = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            ImVec4 colorBright = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

            style.WindowRounding = 0.0f;
            style.FramePadding.y = 8;
            style.ItemSpacing.y = 4;
            style.FrameBorderSize = 3;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
            style.Colors[ImGuiCol_TitleBgActive] = color;

            style.Colors[ImGuiCol_Header]        = color;
            style.Colors[ImGuiCol_HeaderHovered] = colorHovered;
            style.Colors[ImGuiCol_HeaderActive]  = colorActive;

            style.Colors[ImGuiCol_FrameBg]        = color;
            style.Colors[ImGuiCol_FrameBgHovered] = colorHovered;
            style.Colors[ImGuiCol_FrameBgActive]  = colorActive;
            
            style.Colors[ImGuiCol_Tab]               = color;
            style.Colors[ImGuiCol_TabHovered]        = colorHovered;
            style.Colors[ImGuiCol_TabSelected]       = colorActive;
            style.Colors[ImGuiCol_TabDimmed]         = color;
            style.Colors[ImGuiCol_TabDimmedSelected] = color;

            // sets the color of the docking panel
            style.Colors[ImGuiCol_DockingPreview] = colorBright;

            // set the color of the bar above each tab
            style.Colors[ImGuiCol_TabSelectedOverline] = colorBright;

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
        //ImGui::PushFont(mFont);
        ImGuizmo::BeginFrame();
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
    }

    void WindowSystem::FrameEnd_ImGui()
    {
        // Rendering
        //ImGui::PopFont();
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
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

        // Create window with graphics context
        mPrimaryWindow = glfwCreateWindow(1200, 720, "Engine", nullptr, nullptr);
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
        glfwSetScrollCallback(mPrimaryWindow, GLFW_ScrollCallback);
        glfwSetWindowSizeCallback(mPrimaryWindow, GLFW_WindowResizeCallback);
        glfwSetWindowPosCallback(mPrimaryWindow, GLFW_WindowPositionCallback);
        glfwSetDropCallback(mPrimaryWindow, GLFW_DropCallback);

        glewExperimental = GL_TRUE; // Ensure GLEW uses modern techniques for managing OpenGL functionality
        if (glewInit() != GLEW_OK)
        {
            glfwDestroyWindow(mPrimaryWindow);
            glfwTerminate();
            Gep::Log::Critical("Failed To Initialize GLEW");
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

        // checks the system for opengl extensions
        GLint nExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);
        std::unordered_set<std::string> supportedExtensions;
        for (GLint i = 0; i < nExtensions; ++i)
        {
            const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
            supportedExtensions.insert(ext);
        }

        const std::vector<std::string> requiredExtensions = {
            //"GL_ARB_bindless_texture",
        };

        for (const auto& required : requiredExtensions)
        {
            if (!supportedExtensions.contains(required))
            {
                Gep::Log::Critical("This engine requires a missing gpu extension. Current GPU doesn't support, [", required, "]");
            }
        }

        //glEnable(GL_DEBUG_OUTPUT);
        //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        //glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
        //GLsizei length, const GLchar* message, const void* userParam)
        //{
        //    Gep::Log::Info("[OPENGL OUTPUT] ", message);
        //}, nullptr);

        //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    }

    void WindowSystem::FrameStart_GLFW()
    {
        glfwPollEvents();
    }

    void WindowSystem::FrameEnd_GLFW()
    {
        if (glfwWindowShouldClose(mPrimaryWindow))
        {
            // begin the shutdown process for the engine
            mManager.Shutdown();
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
        if (!ws) return;
        
        ws->mManager.SignalEvent<Gep::Event::KeyPressed>({ key, scancode, action, mods });
    }

    void WindowSystem::GLFW_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        WindowSystem* ws = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        if (!ws) return;

        ws->mManager.SignalEvent<Gep::Event::MouseClicked>({ button, action, mods });
    }

    void WindowSystem::GLFW_MousePositionCallback(GLFWwindow* window, double x, double y)
    {
        WindowSystem* ws = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        if (!ws) return;

        static double prevX = 0, prevY = 0;
        ws->mManager.SignalEvent<Gep::Event::MouseMoved>({ x, y, prevX, prevY});
        prevX = x;
        prevY = y;
    }

    void WindowSystem::GLFW_WindowResizeCallback(GLFWwindow* window, int width, int height)
    {
        WindowSystem* ws = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        if (!ws) return;

        ws->mManager.SignalEvent<Gep::Event::WindowResize>({ width, height });
    }

    void WindowSystem::GLFW_WindowPositionCallback(GLFWwindow* window, int x, int y)
    {
        WindowSystem* ws = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        if (!ws) return;

        ws->mManager.SignalEvent<Gep::Event::WindowMoved>({ x, y });
    }

    void WindowSystem::GLFW_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        WindowSystem* ws = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        if (!ws) return;
            
        ws->mManager.SignalEvent<Gep::Event::MouseScrolled>({ xoffset, yoffset });
    }

    void WindowSystem::GLFW_DropCallback(GLFWwindow* window, int count, const char** cpaths)
    {
        WindowSystem* ws = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        if (!ws) return;

        std::vector<std::filesystem::path> paths;
        for (int i = 0; i < count; ++i)
        {
            paths.emplace_back(cpaths[i]);
        }

        ws->mManager.SignalEvent<Gep::Event::FileDropped>({ std::move(paths) });
    }
}



