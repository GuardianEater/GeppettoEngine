/*****************************************************************//**
 * \file   BoundFunction.hpp
 * \brief  easily binds an object and a function
 * 
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

namespace Gep
{
    // binding of non-const member functions
    template <typename ClassType, typename ReturnType, typename... ParamTypes>
    class BoundFunction 
    {
    public:
        using MemberFunctionPtr = ReturnType (ClassType::*)(ParamTypes...);

        BoundFunction(ClassType& boundObject, MemberFunctionPtr memberFunction)
            : mBoundObject(boundObject)
            , mMemberFunction(memberFunction)
        {}

        ReturnType operator()(ParamTypes... params) 
        {
            // dodges warning about returning on void type
            if constexpr (std::is_void<ReturnType>::value)
            {
                (mBoundObject.*mMemberFunction)(params...);
            }
            else
            {
                return (mBoundObject.*mMemberFunction)(params...);
            }
        }

    private:
        ClassType& mBoundObject;
        MemberFunctionPtr mMemberFunction;
    };

    // binding of const member functions
    template <typename ClassType, typename ReturnType, typename... ParamTypes>
    class BoundFunction<const ClassType, ReturnType, ParamTypes...>
    {
    public:
        using ConstMemberFunctionPtr = ReturnType(ClassType::*)(ParamTypes...) const;

        BoundFunction(const ClassType& boundObject, ConstMemberFunctionPtr memberFunction)
            : mBoundObject(boundObject)
            , mMemberFunction(memberFunction)
        {}

        ReturnType operator()(ParamTypes... params) const
        {
            // dodges warning about returning on void type
            if constexpr (std::is_void<ReturnType>::value)
            {
                (mBoundObject.*mMemberFunction)(params...);
            }
            else
            {
                return (mBoundObject.*mMemberFunction)(params...);
            }
        }

    private:
        const ClassType& mBoundObject;
        ConstMemberFunctionPtr mMemberFunction;
    };

}
