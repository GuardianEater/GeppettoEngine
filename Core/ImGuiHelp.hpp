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
typedef int ImGuiDataType;

namespace Gep::ImGui::Detail
{
    // helper for for MultiInputScalar used for creating a inputable field
    template <typename T, typename GetterFunction>
        requires std::is_invocable_v<GetterFunction, T&>
    inline bool MultiInputScalar_Field(std::span<T> values, GetterFunction&& get);

    // helper for for MultiDragScalar used for creating a dragable field
    template <typename T, typename GetterFunction>
        requires std::is_invocable_v<GetterFunction, T&>
    inline bool MultiDragScalar_Field(std::span<T> values, GetterFunction&& get);
}

namespace Gep::ImGui
{
    // wrappers for common imgui functions ///////////////////////////////////////////////////////////////////////////////////////

    // given a type T returns the corresponding ImGuiDataType
    template <typename T>
        requires std::is_arithmetic_v<T>
    inline ImGuiDataType ImGuiTypeID();

    // templated wrapper for imguis input scalar function
    template <typename T>
        requires std::is_arithmetic_v<T>
    inline bool InputScalar(const std::string& label, T* v, const std::string& format = ::ImGui::DataTypeGetInfo(Gep::ImGui::ImGuiTypeID<T>())->PrintFmt, ImGuiInputTextFlags flags = 0);

    // templated wrapper for imguis drag scalar function
    template <typename T>
        requires std::is_arithmetic_v<T>
    inline bool DragScalar(const std::string& label, T* v, float v_speed = 1.0f, T v_min = 0, T v_max = 0, const std::string& format = ::ImGui::DataTypeGetInfo(Gep::ImGui::ImGuiTypeID<T>())->PrintFmt, ImGuiSliderFlags flags = 0);

    // templated wrapper for imguis slider scalar function
    template <typename T>
        requires std::is_arithmetic_v<T>
    inline bool SliderScalar(const std::string& label, T* v, T v_min, T v_max, const std::string& format = ::ImGui::DataTypeGetInfo(Gep::ImGui::ImGuiTypeID<T>())->PrintFmt, ImGuiSliderFlags flags = 0);


    // helpers used for generalized multi-input/drag scalar functions ///////////////////////////////////////////////////////

    // generic form for input scalar, automatically generates input fields corresponding to the amount of getters
    // same as imguis InputScalar but takes a span of Ts instead of a singular and getter functions
    template <typename T, typename... GetterFunctions>
    inline bool MultiInputScalarX(const std::string& label, std::span<T> values, GetterFunctions&&... gets);

    // generic form for drag scalar, automatically generates drag fields corresponding to the amount of getters
    // same as imguis DragScalar but takes a span of Ts instead of a singular and getter functions
    template <typename T, typename... GetterFunctions>
    inline bool MultiDragScalarX(const std::string& label, std::span<T> values, GetterFunctions&&... gets);


    // slider scalar ////////////////////////////////////////////////////////////////

    // same as imguis SliderScalar but takes a span of Ts instead of a singular and a getter function
    template<typename T, typename GetterFunction, typename ScalarType>
        requires std::is_invocable_r_v<ScalarType&, GetterFunction, T&> && std::is_arithmetic_v<ScalarType>
    inline bool MultiSliderScalar(const std::string& label, std::span<T> values, ScalarType min, ScalarType max, GetterFunction&& get);



    // checkboxes /////////////////////////////////////////////////////////////////

    // same as imguis Checkbox but takes a span of Ts instead of a singular and a getter function
    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<bool&, GetterFunction, T&>
    inline bool MultiCheckbox(const std::string& label, std::span<T> values, GetterFunction&& get);



    // drag float /////////////////////////////////////////////////////////////////

    // same as imguis DragFloat but takes a span of Ts instead of a singular and a getter function
    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<float&, GetterFunction, T&>
    inline bool MultiDragFloat(const std::string& label, std::span<T> values, GetterFunction&& get);

    // same as imguis DragFloat2 but takes a span of Ts instead of a singular and 2 getter functions
    template <typename T, typename GetterFunction0, typename GetterFunction1>
        requires std::is_invocable_r_v<float&, GetterFunction0, T&> && std::is_invocable_r_v<float&, GetterFunction1, T&>
    inline bool MultiDragFloat2(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1);

    // same as imguis DragFloat3 but takes a span of Ts instead of a singular and 3 getter functions
    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2>
        requires std::is_invocable_r_v<float&, GetterFunction0, T&> && std::is_invocable_r_v<float&, GetterFunction1, T&> && std::is_invocable_r_v<float&, GetterFunction2, T&>
    inline bool MultiDragFloat3(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2);

    // same as imguis DragFloat4 but takes a span of Ts instead of a singular and 4 getter functions
    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2, typename GetterFunction3>
        requires std::is_invocable_r_v<float&, GetterFunction0, T&> && std::is_invocable_r_v<float&, GetterFunction1, T&> && std::is_invocable_r_v<float&, GetterFunction2, T&> && std::is_invocable_r_v<float&, GetterFunction3, T&>
    inline bool MultiDragFloat4(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2, GetterFunction3&& get3);



    // drag int /////////////////////////////////////////////////////////////////

    // same as imguis DragInt but takes a span of Ts instead of a singular and a getter function
    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<int&, GetterFunction, T&>
    inline bool MultiDragInt(const std::string& label, std::span<T> values, GetterFunction&& get);

    // same as imguis DragInt2 but takes a span of Ts instead of a singular and 2 getter functions
    template <typename T, typename GetterFunction0, typename GetterFunction1>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&>
    inline bool MultiDragInt2(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1);

    // same as imguis DragInt3 but takes a span of Ts instead of a singular and 3 getter functions
    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&> && std::is_invocable_r_v<int&, GetterFunction2, T&>
    inline bool MultiDragInt3(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2);

    // same as imguis DragInt4 but takes a span of Ts instead of a singular and 4 getter functions
    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2, typename GetterFunction3>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&> && std::is_invocable_r_v<int&, GetterFunction2, T&> && std::is_invocable_r_v<int&, GetterFunction3, T&>
    inline bool MultiDragInt4(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2, GetterFunction3&& get3);

    // input int /////////////////////////////////////////////////////////////////

    // same as imguis InputInt but takes a span of Ts instead of a singular and a getter function
    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<int&, GetterFunction, T&>
    inline bool MultiInputInt(const std::string& label, std::span<T> values, GetterFunction&& get);

    // same as imguis InputInt2 but takes a span of Ts instead of a singular and 2 getter functions
    template <typename T, typename GetterFunction0, typename GetterFunction1>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&>
    inline bool MultiInputInt2(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1);

    // same as imguis InputInt3 but takes a span of Ts instead of a singular and 3 getter functions
    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&> && std::is_invocable_r_v<int&, GetterFunction2, T&>
    inline bool MultiInputInt3(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2);

    // same as imguis InputInt4 but takes a span of Ts instead of a singular and 4 getter functions
    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2, typename GetterFunction3>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&> && std::is_invocable_r_v<int&, GetterFunction2, T&> && std::is_invocable_r_v<int&, GetterFunction3, T&>
    inline bool MultiInputInt4(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2, GetterFunction3&& get3);
}

#include "ImGuiHelp.inl"
