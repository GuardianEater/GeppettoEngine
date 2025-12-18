/*****************************************************************//**
 * \file   ImGuiHelp.hpp
 * \brief  abunch of wrapper helpers for using ImGui
 * 
 * \author 2018t
 * \date   December 2025
 *********************************************************************/

#pragma once

typedef int ImGuiSliderFlags;
typedef int ImGuiColorEditFlags;

namespace Gep::ImGui
{
    template <typename T, typename GetterFunction>
        requires std::is_invocable_v<GetterFunction, T&>
    bool ValuesAreUniform(std::span<T> values, GetterFunction&& get);

    template <typename T>
        requires std::is_arithmetic_v<T>
    ImGuiDataType ImGuiTypeID();

    template <typename T>
        requires std::is_arithmetic_v<T>
    bool DragScalar(const std::string& label, T* v, float v_speed = 1.0f, T v_min = 0, T v_max = 0, const std::string& format = "%.3f", ImGuiSliderFlags flags = 0);


    // helper for for MultiDragFloat used for creating a dragable field
    template <typename T, typename GetterFunction>
        requires std::is_invocable_v<GetterFunction, T&>
    bool MultiDragScalar_Field(std::span<T> values, GetterFunction&& get);

    template <typename T, typename... GetterFunctions>
    bool MultiDragScalar_Impl(const std::string& label, std::span<T> values, GetterFunctions&&... gets);

    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<float&, GetterFunction, T&>
    bool MultiDragFloat(const std::string& label, std::span<T> values, GetterFunction&& get);

    template <typename T, typename GetterFunction0, typename GetterFunction1>
        requires std::is_invocable_r_v<float&, GetterFunction0, T&> && std::is_invocable_r_v<float&, GetterFunction1, T&>
    bool MultiDragFloat2(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1);

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2>
        requires std::is_invocable_r_v<float&, GetterFunction0, T&> && std::is_invocable_r_v<float&, GetterFunction1, T&> && std::is_invocable_r_v<float&, GetterFunction2, T&>
    bool MultiDragFloat3(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2);

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2, typename GetterFunction3>
        requires std::is_invocable_r_v<float&, GetterFunction0, T&>&& std::is_invocable_r_v<float&, GetterFunction1, T&>&& std::is_invocable_r_v<float&, GetterFunction2, T&>&& std::is_invocable_r_v<float&, GetterFunction3, T&>
    bool MultiDragFloat4(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2, GetterFunction3&& get3);
}

#include "ImGuiHelp.inl"
