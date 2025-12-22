/*****************************************************************//**
 * \file   ImGuiHelp.inl
 * \brief  abunch of wrapper helpers for using ImGui
 * 
 * \author 2018t
 * \date   December 2025
 *********************************************************************/

#pragma once

#include "STLHelp.hpp"
#include "ImGuiHelp.hpp"

namespace Gep::ImGui
{
    template <typename T>
        requires std::is_arithmetic_v<T>
    ImGuiDataType ImGuiTypeID()
    {
        if constexpr (std::is_same_v<T, float>)
            return ImGuiDataType_Float;
        else if constexpr (std::is_same_v<T, double>)
            return ImGuiDataType_Double;
        else if constexpr (std::is_same_v<T, int8_t>)
            return ImGuiDataType_S8;
        else if constexpr (std::is_same_v<T, uint8_t>)
            return ImGuiDataType_U8;
        else if constexpr (std::is_same_v<T, int16_t>)
            return ImGuiDataType_S16;
        else if constexpr (std::is_same_v<T, uint16_t>)
            return ImGuiDataType_U16;
        else if constexpr (std::is_same_v<T, int32_t>)
            return ImGuiDataType_S32;
        else if constexpr (std::is_same_v<T, uint32_t>)
            return ImGuiDataType_U32;
        else if constexpr (std::is_same_v<T, int64_t>)
            return ImGuiDataType_S64;
        else //if constexpr (std::is_same_v<T, uint64_t>)
            return ImGuiDataType_U64;
    }

    template <typename T>
        requires std::is_arithmetic_v<T>
    bool DragScalar(const std::string& label, T* v, float v_speed, T min, T max, const std::string& format, ImGuiSliderFlags flags)
    {
        return ::ImGui::DragScalar(label.c_str(), Gep::ImGui::ImGuiTypeID<T>(), static_cast<void*>(v), v_speed, static_cast<void*>(&min), static_cast<void*>(&max), format.c_str(), flags);
    }

    template<typename T>
        requires std::is_arithmetic_v<T>
    bool SliderScalar(const std::string& label, T* v, T v_min, T v_max, const std::string& format, ImGuiSliderFlags flags)
    {
        return ::ImGui::SliderScalar(label.c_str(), Gep::ImGui::ImGuiTypeID<T>(), static_cast<void*>(v), static_cast<void*>(&v_min), static_cast<void*>(&v_max), format.c_str(), flags);
    }

    template <typename T>
        requires std::is_arithmetic_v<T>
    bool InputScalar(const std::string& label, T* v, const std::string& format, ImGuiInputTextFlags flags)
    {
        return ::ImGui::InputScalar(label.c_str(), Gep::ImGui::ImGuiTypeID<T>(), static_cast<void*>(v), nullptr, nullptr, format.c_str(), flags | ImGuiInputTextFlags_EnterReturnsTrue);
    }

    template <typename T, typename GetterFunction>
        requires std::is_invocable_v<GetterFunction, T&>
    bool MultiDragScalar_Field(std::span<T> values, GetterFunction&& get)
    {
        using ScalarType = std::remove_reference_t<decltype(get(values.front()))>;

        bool uniform = Gep::IsUniform(values, get);
        bool changed = false;
        if (uniform)
        {
            ScalarType front = get(values.front());
            changed = Gep::ImGui::DragScalar("", &front, ScalarType(0.1f));
            if (changed)
            {
                for (T& v : values)
                    get(v) = front;
            }
        }
        else
        {
            ScalarType delta = 0;
            changed = Gep::ImGui::DragScalar("", &delta, ScalarType(0.1f), ScalarType(0), ScalarType(0), "-");

            if (changed)
            {
                if (::ImGui::IsItemActive() && ::ImGui::IsMouseDragging(::ImGuiMouseButton_Left))
                {
                    for (T& v : values)
                        get(v) += delta;
                }
                else if (::ImGui::IsItemActive() && ::ImGui::TempInputIsActive(::ImGui::GetActiveID()))
                {
                    for (T& v : values)
                        get(v) = delta;
                }
            }
        }

        return changed;
    }

    template <typename T, typename GetterFunction>
        requires std::is_invocable_v<GetterFunction, T&>
    bool MultiInputScalar_Field(std::span<T> values, GetterFunction&& get)
    {
        using ScalarType = std::remove_reference_t<decltype(get(values.front()))>;

        bool uniform = Gep::IsUniform(values, get);
        bool changed = false;
        if (uniform)
        {
            ScalarType front = get(values.front());
            changed = Gep::ImGui::InputScalar("", &front);
            if (changed)
            {
                for (T& v : values)
                    get(v) = front;
            }
        }
        else
        {
            ScalarType delta = 0;
            changed = Gep::ImGui::InputScalar("", &delta, "-");

            if (changed)
            {
                // this is here for input scalar buttons that are created in the default imgui InputScalar.
                if (::ImGui::IsItemActive() && ::ImGui::TempInputIsActive(::ImGui::GetActiveID()))
                {
                    for (T& v : values)
                        get(v) = delta;
                }
                else
                {
                    for (T& v : values)
                        get(v) += delta;
                }
            }
        }

        return changed;
    }

    template <typename T, typename... GetterFunctions>
    bool MultiDragScalarX(const std::string& label, std::span<T> values, GetterFunctions&&... gets)
    {
        constexpr size_t getsCount = sizeof...(gets);

        ::ImGui::BeginGroup();
        ::ImGui::PushMultiItemsWidths(getsCount, ::ImGui::CalcItemWidth());
        ::ImGui::PushID(label.c_str());

        size_t i = 0;
        bool anyChanged = false;
        ([&](auto& get)
        {
            ::ImGui::PushID(static_cast<int>(i));
            if (i > 0)
                ::ImGui::SameLine(0, ::ImGui::GetStyle().ItemInnerSpacing.x);

            anyChanged |= Gep::ImGui::MultiDragScalar_Field(values, get);
            ::ImGui::PopItemWidth();

            ::ImGui::PopID();
            ++i;
        }
        (gets), ...);

        ::ImGui::PopID();

        const char* label_end = ::ImGui::FindRenderedTextEnd(label.c_str());
        if (label.c_str() != label_end)
        {
            ::ImGui::SameLine(0, ::ImGui::GetStyle().ItemInnerSpacing.x);
            ::ImGui::TextEx(label.c_str(), label_end);
        }
        ::ImGui::EndGroup();

        return anyChanged;
    }

    template<typename T, typename GetterFunction, typename ScalarType>
        requires std::is_invocable_r_v<ScalarType&, GetterFunction, T&> && std::is_arithmetic_v<ScalarType>
    bool MultiSliderScalar(const std::string& label, std::span<T> values, ScalarType min, ScalarType max, GetterFunction&& get)
    {
        if (values.empty())
            return false;

        bool uniform = Gep::IsUniform(values, get);
        bool changed = false;
        if (uniform)
        {
            ScalarType front = get(values.front());
            changed = Gep::ImGui::SliderScalar(label.c_str(), &front, min, max);
            if (changed)
            {
                for (T& v : values)
                    get(v) = front;
            }
        }
        else
        {
            ScalarType delta = min;
            changed = Gep::ImGui::SliderScalar(label.c_str(), &delta, min, max, "-");
            if (changed)
            {
                for (T& v : values)
                    get(v) = delta;
            }
        }

        return changed;
    }

    template <typename T, typename... GetterFunctions>
    bool MultiInputScalarX(const std::string& label, std::span<T> values, GetterFunctions&&... gets)
    {
        constexpr size_t getsCount = sizeof...(gets);

        ::ImGui::BeginGroup();
        ::ImGui::PushMultiItemsWidths(getsCount, ::ImGui::CalcItemWidth());
        ::ImGui::PushID(label.c_str());

        size_t i = 0;
        bool anyChanged = false;
        ([&](auto& get)
        {
            ::ImGui::PushID(static_cast<int>(i));
            if (i > 0)
                ::ImGui::SameLine(0, ::ImGui::GetStyle().ItemInnerSpacing.x);

            anyChanged |= Gep::ImGui::MultiInputScalar_Field(values, get);
            ::ImGui::PopItemWidth();

            ::ImGui::PopID();
            ++i;
        }
        (gets), ...);

        ::ImGui::PopID();

        const char* label_end = ::ImGui::FindRenderedTextEnd(label.c_str());
        if (label.c_str() != label_end)
        {
            ::ImGui::SameLine(0, ::ImGui::GetStyle().ItemInnerSpacing.x);
            ::ImGui::TextEx(label.c_str(), label_end);
        }
        ::ImGui::EndGroup();

        return anyChanged;
    }

    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<bool&, GetterFunction, T&>
    bool MultiCheckbox(const std::string& label, std::span<T> values, GetterFunction&& get)
    {
        bool changed = false;

        ::ImGui::BeginGroup();
        ::ImGui::PushID(label.c_str());

        bool uniform = Gep::IsUniform(values, get);
        if (uniform)
        {
            bool front = get(values.front());
            if (::ImGui::Checkbox(label.c_str(), &front))
            {
                changed = true;
                for (T& v : values)
                    get(v) = front;
            }
        }
        else
        {
            ::ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
            bool mixedValue = false;

            if (::ImGui::Checkbox(label.c_str(), &mixedValue))
            {
                changed = true;
                for (T& v : values)
                    get(v) = true;
            }

            ::ImGui::PopItemFlag();
        }


        ::ImGui::PopID();
        ::ImGui::EndGroup();

        return changed;
    }

    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<float&, GetterFunction, T&>
    bool MultiDragFloat(const std::string& label, std::span<T> values, GetterFunction&& get)
    {
        return Gep::ImGui::MultiDragScalarX(label, values, std::forward<GetterFunction>(get));
    }

    template <typename T, typename GetterFunction0, typename GetterFunction1>
        requires std::is_invocable_r_v<float&, GetterFunction0, T&>&& std::is_invocable_r_v<float&, GetterFunction1, T&>
    bool MultiDragFloat2(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1)
    {
        return Gep::ImGui::MultiDragScalarX(label, values, std::forward<GetterFunction0>(get0), std::forward<GetterFunction1>(get1));
    }

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2>
        requires std::is_invocable_r_v<float&, GetterFunction0, T&>&& std::is_invocable_r_v<float&, GetterFunction1, T&>&& std::is_invocable_r_v<float&, GetterFunction2, T&>
    bool MultiDragFloat3(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2)
    {
        return Gep::ImGui::MultiDragScalarX(label, values, std::forward<GetterFunction0>(get0), std::forward<GetterFunction1>(get1), std::forward<GetterFunction2>(get2));
    }

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2, typename GetterFunction3>
        requires std::is_invocable_r_v<float&, GetterFunction0, T&>&& std::is_invocable_r_v<float&, GetterFunction1, T&>&& std::is_invocable_r_v<float&, GetterFunction2, T&>&& std::is_invocable_r_v<float&, GetterFunction3, T&>
    bool MultiDragFloat4(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2, GetterFunction3&& get3)
    {
        return Gep::ImGui::MultiDragScalarX(label, values, std::forward<GetterFunction0>(get0), std::forward<GetterFunction1>(get1), std::forward<GetterFunction2>(get2), std::forward<GetterFunction3>(get3));
    }


    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<int&, GetterFunction, T&>
    bool MultiDragInt(const std::string& label, std::span<T> values, GetterFunction&& get)
    {
        return Gep::ImGui::MultiDragScalarX(label, values, std::forward<GetterFunction>(get));
    }

    template <typename T, typename GetterFunction0, typename GetterFunction1>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&>&& std::is_invocable_r_v<int&, GetterFunction1, T&>
    bool MultiDragInt2(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1)
    {
        return Gep::ImGui::MultiDragScalarX(label, values, std::forward<GetterFunction0>(get0), std::forward<GetterFunction1>(get1));
    }

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&>&& std::is_invocable_r_v<int&, GetterFunction1, T&>&& std::is_invocable_r_v<int&, GetterFunction2, T&>
    bool MultiDragInt3(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2)
    {
        return Gep::ImGui::MultiDragScalarX(label, values, std::forward<GetterFunction0>(get0), std::forward<GetterFunction1>(get1), std::forward<GetterFunction2>(get2));
    }

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2, typename GetterFunction3>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&>&& std::is_invocable_r_v<int&, GetterFunction1, T&>&& std::is_invocable_r_v<int&, GetterFunction2, T&>&& std::is_invocable_r_v<int&, GetterFunction3, T&>
    bool MultiDragInt4(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2, GetterFunction3&& get3)
    {
        return Gep::ImGui::MultiDragScalarX(label, values, std::forward<GetterFunction0>(get0), std::forward<GetterFunction1>(get1), std::forward<GetterFunction2>(get2), std::forward<GetterFunction3>(get3));
    }


    template <typename T, typename GetterFunction>
        requires std::is_invocable_r_v<int&, GetterFunction, T&>
    bool MultiInputInt(const std::string& label, std::span<T> values, GetterFunction&& get)
    {
        return Gep::ImGui::MultiInputScalarX(label, values, std::forward<GetterFunction>(get));
    }

    template <typename T, typename GetterFunction0, typename GetterFunction1>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&>
    bool MultiInputInt2(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1)
    {
        return Gep::ImGui::MultiInputScalarX(label, values, std::forward<GetterFunction0>(get0), std::forward<GetterFunction1>(get1));
    }

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&> && std::is_invocable_r_v<int&, GetterFunction2, T&>
    bool MultiInputInt3(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2)
    {
        return Gep::ImGui::MultiInputScalarX(label, values, std::forward<GetterFunction0>(get0), std::forward<GetterFunction1>(get1), std::forward<GetterFunction2>(get2));
    }

    template <typename T, typename GetterFunction0, typename GetterFunction1, typename GetterFunction2, typename GetterFunction3>
        requires std::is_invocable_r_v<int&, GetterFunction0, T&> && std::is_invocable_r_v<int&, GetterFunction1, T&> && std::is_invocable_r_v<int&, GetterFunction2, T&> && std::is_invocable_r_v<int&, GetterFunction3, T&>
    bool MultiInputInt4(const std::string& label, std::span<T> values, GetterFunction0&& get0, GetterFunction1&& get1, GetterFunction2&& get2, GetterFunction3&& get3)
    {
        return Gep::ImGui::MultiInputScalarX(label, values, std::forward<GetterFunction0>(get0), std::forward<GetterFunction1>(get1), std::forward<GetterFunction2>(get2), std::forward<GetterFunction3>(get3));
    }

} // namespace Gep::ImGui
