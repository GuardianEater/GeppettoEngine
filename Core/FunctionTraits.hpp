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
    // a bunch of helpers to extract type information from an arbitrary callable
    template<typename T>
    struct FunctionTraits;

    // function pointer
    template<typename Ret, typename... Args>
    struct FunctionTraits<Ret(*)(Args...)> {
        using ReturnType = Ret;
        using ArgumentsTypeList = Gep::TypeList<Args...>;
    };

    // function reference
    template<typename Ret, typename... Args>
    struct FunctionTraits<Ret(Args...)> {
        using ReturnType = Ret;
        using ArgumentsTypeList = Gep::TypeList<Args...>;
    };

    // std::function
    template<typename Ret, typename... Args>
    struct FunctionTraits<std::function<Ret(Args...)>> {
        using ReturnType = Ret;
        using ArgumentsTypeList = Gep::TypeList<Args...>;
    };

    // member function pointer
    template<typename Ret, typename ClassType, typename... Args>
    struct FunctionTraits<Ret(ClassType::*)(Args...)> {
        using ReturnType = Ret;
        using ArgumentsTypeList = Gep::TypeList<Args...>;
    };

    // const member function pointer
    template<typename Ret, typename ClassType, typename... Args>
    struct FunctionTraits<Ret(ClassType::*)(Args...) const> {
        using ReturnType = Ret;
        using ArgumentsTypeList = Gep::TypeList<Args...>;
    };

    // lambda or functor
    template<typename T>
    struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};
}
