/*****************************************************************//**
 * \file   ImGuiSystem.hpp
 * \brief  System that operates imgui
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

// backend
#include <imgui.h>
#include <System.hpp>

// client

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
	};
}
