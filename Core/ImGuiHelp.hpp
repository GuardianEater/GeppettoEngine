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
    // wrappers for common imgui functions ///////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
        requires std::is_arithmetic_v<T>
    ImGuiDataType ImGuiTypeID();

    template <typename T>
        requires std::is_arithmetic_v<T>
    bool InputScalar(const std::string& label, T* v, const std::string& format = ::ImGui::DataTypeGetInfo(Gep::ImGui::ImGuiTypeID<T>())->PrintFmt, ImGuiInputTextFlags flags = 0);

    template <typename T>
        requires std::is_arithmetic_v<T>
    bool DragScalar(const std::string& label, T* v, float v_speed = 1.0f, T v_min = 0, T v_max = 0, const std::string& format = ::ImGui::DataTypeGetInfo(Gep::ImGui::ImGuiTypeID<T>())->PrintFmt, ImGuiSliderFlags flags = 0);

    template <typename T>
        requires std::is_arithmetic_v<T>
    bool SliderScalar(const std::string& label, T* v, T v_min, T v_max, const std::string& format = ::ImGui::DataTypeGetInfo(Gep::ImGui::ImGuiTypeID<T>())->PrintFmt, ImGuiSliderFlags flags = 0);


    // helpers used for generalized multi-input/drag scalar functions ///////////////////////////////////////////////////////

    // helper for for MultiInputScalar used for creating a inputable field
    template <typename T, typename GetterFunction>
        requires std::is_invocable_v<GetterFunction, T&>
    bool MultiInputScalar_Field(std::span<T> values, GetterFunction&& get);

    template <typename T, typename... GetterFunctions>
    bool MultiInputScalarX(const std::string& label, std::span<T> values, GetterFunctions&&... gets);


    // helper for for MultiDragScalar used for creating a dragable field
    template <typename T, typename GetterFunction>
        requires std::is_invocable_v<GetterFunction, T&>
    bool MultiDragScalar_Field(std::span<T> values, GetterFunction&& get);

    template <typename T, typename... GetterFunctions>
    bool MultiDragScalarX(const std::string& label, std::span<T> values, GetterFunctions&&... gets);


    // helper
    template<typename T, typename GetterFunction, typename ScalarType>
        requires std::is_invocable_r_v<ScalarType&, GetterFunction, T&> && std::is_arithmetic_v<ScalarType>
    bool MultiSliderScalar(const std::string& label, std::span<T> values, ScalarType min, ScalarType max, GetterFunction&& get);

    // specific implementations for common types ///////////////////////////////////////////////////////////////////////////////////////

    // checkboxes /////////////////////////////////////////////////////////////////

    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<bool&, GetterFunction, T&>
    bool MultiCheckbox(const std::string& label, std::span<T> values, GetterFunction&& get);


    // drag float /////////////////////////////////////////////////////////////////

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
        requires std::is_invocable_r_v<float&, GetterFunction0, T&> && std::is_invocable_r_v<float&, GetterFunction1, T&> && std::is_invocable_r_v<float&, GetterFunction2, T&> && std::is_invocable_r_v<float&, GetterFunction3, T&>
    bool MultiDragFloat4(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2, GetterFunction3&& get3);

    // drag int /////////////////////////////////////////////////////////////////

    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<int&, GetterFunction, T&>
    bool MultiDragInt(const std::string& label, std::span<T> values, GetterFunction&& get);

    template <typename T, typename GetterFunction0, typename GetterFunction1>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&>
    bool MultiDragInt2(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1);

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&> && std::is_invocable_r_v<int&, GetterFunction2, T&>
    bool MultiDragInt3(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2);

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2, typename GetterFunction3>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&> && std::is_invocable_r_v<int&, GetterFunction2, T&> && std::is_invocable_r_v<int&, GetterFunction3, T&>
    bool MultiDragInt4(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2, GetterFunction3&& get3);

    // input int /////////////////////////////////////////////////////////////////

    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<int&, GetterFunction, T&>
    bool MultiInputInt(const std::string& label, std::span<T> values, GetterFunction&& get);

    template <typename T, typename GetterFunction0, typename GetterFunction1>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&>
    bool MultiInputInt2(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1);

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&> && std::is_invocable_r_v<int&, GetterFunction2, T&>
    bool MultiInputInt3(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2);

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2, typename GetterFunction3>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&> && std::is_invocable_r_v<int&, GetterFunction2, T&> && std::is_invocable_r_v<int&, GetterFunction3, T&>
    bool MultiInputInt4(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2, GetterFunction3&& get3);
}

#include "ImGuiHelp.inl"
