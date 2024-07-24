/*****************************************************************//**
 * \file   ImGuiSystem.hpp
 * \brief  System that operates imgui
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <System.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

namespace Client
{
	struct ImGuiComponent
	{

	};

	class ImGuiSystem : public Gep::ISystem
	{
	public:
		ImGuiSystem(Gep::EngineManager& em)
			: ISystem(em)
		{}

        void Init() override
        {
            
        }

	};
}
