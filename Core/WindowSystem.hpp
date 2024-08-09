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
#include <System.hpp>

// external
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// client
#include <Identification.hpp>
#include <Transform.hpp>
#include <Material.hpp>
#include <RigidBody.hpp>


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
            ///////////////////////////////////////////////////////////////////////////////////////
            /// GLFW setup ////////////////////////////////////////////////////////////////////////

            glfwSetErrorCallback(_glfw_error_callback);
            assert(glfwInit() && "Failed To Create Window");

            const char* glsl_version = "#version 430";
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


            ///////////////////////////////////////////////////////////////////////////////////////
            /// ImGui setup ///////////////////////////////////////////////////////////////////////

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

        void Update(float dt) override
        {
            if (glfwWindowShouldClose(mPrimaryWindow)) mManager.ExitEngine();

            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            //////////////////////////////////////
            // do rendering here

            DrawEntitiesWindow();

            DrawUtilitiesWindow(dt);

            //////////////////////////////////////
            
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

            glfwSwapBuffers(mPrimaryWindow);
        }

    private:
        inline void DrawEntitiesWindow()
        {
            ImGui::Begin("Entities");
            for (Gep::Entity entity : mEntities)
            {
                Identification& identification = mManager.GetComponent<Identification>(entity);

                bool hasTransform = mManager.HasComponent<Transform>(entity);
                bool hasRigidBody = mManager.HasComponent<RigidBody>(entity);
                bool hasMaterial = mManager.HasComponent<Material>(entity);
                bool hasIdentification = mManager.HasComponent<Identification>(entity);

                std::string name;
                if (hasIdentification)
                {
                    name = identification.name;
                }

                if (ImGui::CollapsingHeader((name + "##" + std::to_string(entity)).c_str()))
                {
                    ImGui::Begin((name + "##" + std::to_string(entity)).c_str());

                    if (hasIdentification)
                    {
                        if (ImGui::CollapsingHeader(typeid(Identification).name()))
                        {
                            static char newName[16];
                            if (ImGui::InputText("Name", newName, sizeof(newName), ImGuiInputTextFlags_EnterReturnsTrue))
                            {
                                strcpy(&identification.name[0], newName);
                            }
                        }
                    }

                    if (hasTransform)
                    {
                        if (ImGui::CollapsingHeader(typeid(Transform).name()))
                        {
                            Transform& transform = mManager.GetComponent<Transform>(entity);
                            ImGui::DragFloat3("Position", &transform.position[0]);
                            ImGui::DragFloat3("Scale", &transform.scale[0]);
                            ImGui::DragFloat3("Rotation Axis", &transform.rotationAxis[0]);
                            ImGui::DragFloat("Rotation", &transform.rotationAmount);
                        }
                    }

                    if (hasRigidBody)
                    {
                        if (ImGui::CollapsingHeader(typeid(RigidBody).name()))
                        {
                            RigidBody& rigidbody = mManager.GetComponent<RigidBody>(entity);
                            ImGui::DragFloat3("Velocity", &rigidbody.velocity[0]);
                            ImGui::DragFloat3("Acceleration", &rigidbody.acceleration[0]);
                            ImGui::DragFloat("Rotational Velocity", &rigidbody.rotationalVelocity);
                        }
                    }

                    if (hasMaterial)
                    {
                        if (ImGui::CollapsingHeader(typeid(Material).name()))
                        {
                            Material& material = mManager.GetComponent<Material>(entity);
                            ImGui::DragFloat3("Specular Color", &material.spec_coeff[0]);
                            ImGui::DragFloat("Shine", &material.spec_exponent);
                            ImGui::ColorPicker3("Color", &material.diff_coeff[0]);
                        }
                    }

                    ImGui::End();
                }
            }
            ImGui::End();
        }

        inline void DrawUtilitiesWindow(float dt)
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

        // automatically logs input data. the size is the amount of frames of data it will log 
        template <size_t Size>
        void DrawFrameTimes(float dt)
        {
            static std::array<float, Size> frameTimes;
            static float frameTimeSum = 0.0f;
            static size_t frameIndex = 0;

            float frameTime = dt;
            frameTimeSum -= frameTimes[frameIndex];
            frameTimes[frameIndex] = frameTime;
            frameTimeSum += frameTime;

            // Increment index and wrap around if necessary
            frameIndex = (frameIndex + 1) % frameTimes.size();

            ImGui::PlotHistogram("Frames Times", frameTimes.data(), frameTimes.size(), frameIndex, nullptr, 0, 50, ImVec2(0, 80));
        }

        // Size is the amount of frame counts to store
        template <size_t Size>
        void DrawFpsLog(float dt)
        {
            static std::array<float, Size> frameCounts; // each index is an amount of frames
            static float timePassed = 0.0f; // used for timing one second
            static float frames = 0;        // incremented everyframe until second exceeds 1
            static size_t currentIndex = 0; // keeps track of the where the histogram is drawing to
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

            // calculation for the fps average
            if (!isDataLogFull && currentIndex < averageTimeFrame)
            {
                ImGui::Text("Average FPS: Calculating...");
                ImGui::Text("Minimum FPS: Calculating...");
            }
            else
            {
                // thus negative
                // current index = 9
                // atf = 10
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

        GLFWwindow* mPrimaryWindow;
        ImGuiIO* mIO;
	};
}
