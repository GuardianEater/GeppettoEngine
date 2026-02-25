/*****************************************************************//**
 * \file   FunctionTraits.hpp
 * \brief  a bunch of helpers to extract type information from an arbitrary function type
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include <functional>
#include "gtl/type_list.hpp"

namespace Gep
{
    // a bunch of helpers to extract type information from an arbitrary callable
    template<typename T>
    struct FunctionTraits;

    // function pointer
    template<typename Ret, typename... Args>
    struct FunctionTraits<Ret(*)(Args...)> {
        using ReturnType = Ret;
        using ArgumentsTypeList = gtl::type_list<Args...>;
    };

    // function reference
    template<typename Ret, typename... Args>
    struct FunctionTraits<Ret(Args...)> {
        using ReturnType = Ret;
        using ArgumentsTypeList = gtl::type_list<Args...>;
    };

    // std::function
    template<typename Ret, typename... Args>
    struct FunctionTraits<std::function<Ret(Args...)>> {
        using ReturnType = Ret;
        using ArgumentsTypeList = gtl::type_list<Args...>;
    };

    // member function pointer
    template<typename Ret, typename ClassType, typename... Args>
    struct FunctionTraits<Ret(ClassType::*)(Args...)> {
        using ReturnType = Ret;
        using ArgumentsTypeList = gtl::type_list<Args...>;
    };

    // const member function pointer
    template<typename Ret, typename ClassType, typename... Args>
    struct FunctionTraits<Ret(ClassType::*)(Args...) const> {
        using ReturnType = Ret;
        using ArgumentsTypeList = gtl::type_list<Args...>;
    };

    // lambda or functor
    template<typename T>
    struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};
}
