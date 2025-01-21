/*****************************************************************//**
 * \file   WindowSystem.cpp
 * \brief  Used to handle the window
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "WindowSystem.hpp"
#include <numeric>
#include <algorithm>

namespace Client
{
    WindowSystem::WindowSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
    }

    void WindowSystem::Update(float dt)
    {
        //DrawEntitiesWindow();
        DrawUtilitiesWindow(dt);
        //DrawMeshesWindow();
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
}



