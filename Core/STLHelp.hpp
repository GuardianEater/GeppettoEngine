/*****************************************************************//**
 * \file   STLHelp.hpp
 * \brief  helper functions to wrap common stl operations
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include <vector>

namespace Gep
{
    // performs erase remove idiom on a vector; removes the given value from the vector
    template <typename Type>
    void EraseRemove(std::vector<Type>& vec, const Type& value);

    // if looping is false behaves the same as clamp, however if looping is true, will wrap around if the number goes out of range
    template <typename Type>
        requires std::is_floating_point_v<Type>
    Type WrapOrClamp(const Type value, const Type min, const Type max, bool looping);

    template <typename T, typename GetterFunction>
        requires std::is_invocable_v<GetterFunction, const T&> && std::equality_comparable<std::invoke_result_t<GetterFunction, const T&>>
    bool IsUniform(std::span<const T> values, GetterFunction&& get = [](const T& v) { return v; });

    void ReplaceAll(std::string& str, const std::string& from, const std::string& to);
}

namespace Gep
{
    template<typename Type>
    using NakedType = std::remove_cv_t<std::remove_pointer_t<std::remove_cvref_t<Type>>>;

    template<typename T>
    constexpr decltype(auto) DereferenceIfPointer(T&& value)
    {
        if constexpr (std::is_pointer_v<std::remove_reference_t<T>>)
            return *value;
        else
            return std::forward<T>(value);
    }

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

    template <typename Type, typename PointerToMemberType>
    concept IsMemberOf = requires(Type&& obj, PointerToMemberType&& memberPtr)
    {
        { Gep::DereferenceIfPointer(obj).*memberPtr };
    };

    // given an array of objects, makes a vector of the given member
    template <typename ObjectType, typename PointerToMemberType>
        requires IsMemberOf<ObjectType, PointerToMemberType>
    auto PackMembers(const std::span<ObjectType> vec, PointerToMemberType&& memberPtr)
    {
        using NakedType = Gep::NakedType<ObjectType>;
        using MemberType = std::decay_t<decltype(std::declval<NakedType>().*memberPtr)>;

        std::vector<MemberType> memberVec;
        memberVec.reserve(vec.size());
        for (const ObjectType& object : vec) // at this point object could be a pointer or a reference
        {
            const auto& ref = Gep::DereferenceIfPointer(object);
            memberVec.push_back(ref.*memberPtr);
        }

        return memberVec;
    }

    // given an array of objects, applies the values to the location of the member pointer index by index
    template <typename ObjectType, typename PointerToMemberType, typename MemberType>
        requires IsMemberOf<ObjectType, PointerToMemberType>
    void UnpackMembers(const std::span<ObjectType> vec, PointerToMemberType&& memberPtr, const std::vector<MemberType>& values)
    {
        // must be the same size
        if (vec.size() != values.size())
            return;

        for (size_t i = 0; i < vec.size(); ++i)
        {
            auto& ref = Gep::DereferenceIfPointer(vec[i]);
            ref.*memberPtr = values[i];
        }
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