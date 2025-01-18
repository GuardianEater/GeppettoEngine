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
#include <Identification.hpp>
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

        void Update(float dt);

    private:
        void DrawMeshesWindow();
        void DrawEntitiesWindow();
        void DrawUtilitiesWindow(float dt);

        template <size_t Size>
        void DrawFrameTimes(float dt);

        template <size_t Size>
        void DrawFpsLog(float dt);
    };
}



