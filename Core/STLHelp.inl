/*****************************************************************//**
 * \file   STLHelp.inl
 * \brief  template implementations for STL helper functions
 * 
 * \author 2018t
 * \date   February 2026
 *********************************************************************/

#pragma once

namespace Gep
{
    template<typename Type>
    using NakedType = std::remove_cv_t<std::remove_pointer_t<std::remove_cvref_t<Type>>>;

    template<typename Type>
    void EraseRemove(std::vector<Type>& vec, const Type& value)
    {
        vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
    }

    template <typename Type>
        requires std::is_floating_point_v<Type>
    Type WrapOrClamp(const Type value, const Type min, const Type max, bool looping)
    {
        Type result{};

        if (looping)
        {
            result = std::fmod(value, max);
            if (result < min) result += max;
        }
        else
        {
            result = std::clamp(value, min, max);
        }

        return result;
    }

    // simple checker that returns true if all values in the span are the same according to the getter function
    template <typename T, typename GetterFunction>
        requires std::is_invocable_v<GetterFunction, T&>
    bool IsUniform(std::span<T> values, GetterFunction&& get)
    {
        if (values.size() <= 1)
            return true;

        const auto& first = get(values.front());
        for (size_t i = 1; i < values.size(); ++i)
        {
            if (get(values[i]) != first)
                return false;
        }

        return true;
    }
}
