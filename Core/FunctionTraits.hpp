/*****************************************************************//**
 * \file   FunctionTraits.hpp
 * \brief  a bunch of helpers to extract type information from an arbitrary function type
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include <functional>
#include "TypeList.hpp"

namespace Gep
{
    template<typename T>
    struct FunctionTraits;

    // Function pointer
    template<typename Ret, typename... Args>
    struct FunctionTraits<Ret(*)(Args...)> {
        using ReturnType = Ret;
        using ArgumentsTypeList = Gep::type_list<Args...>;
    };

    // Function reference
    template<typename Ret, typename... Args>
    struct FunctionTraits<Ret(Args...)> {
        using ReturnType = Ret;
        using ArgumentsTypeList = Gep::type_list<Args...>;
    };

    // std::function
    template<typename Ret, typename... Args>
    struct FunctionTraits<std::function<Ret(Args...)>> {
        using ReturnType = Ret;
        using ArgumentsTypeList = Gep::type_list<Args...>;
    };

    // Member function pointer
    template<typename Ret, typename ClassType, typename... Args>
    struct FunctionTraits<Ret(ClassType::*)(Args...)> {
        using ReturnType = Ret;
        using ArgumentsTypeList = Gep::type_list<Args...>;
    };

    // Const member function pointer
    template<typename Ret, typename ClassType, typename... Args>
    struct FunctionTraits<Ret(ClassType::*)(Args...) const> {
        using ReturnType = Ret;
        using ArgumentsTypeList = Gep::type_list<Args...>;
    };

    // Lambda or functor
    template<typename T>
    struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};
}
