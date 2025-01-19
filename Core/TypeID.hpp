/*****************************************************************//**
 * \file   TypeID.hpp
 * \brief  
 * 
 * \author Travis Gronvold
 * \date   January 2025
 *********************************************************************/

#pragma once

#include <typeindex>
#include <string>

namespace Gep
{
    class TypeInfo
    {
    public:
        TypeInfo() = default;
        TypeInfo(const TypeInfo&) = default;
        TypeInfo(std::type_index typeIndex)
            : mTypeIndex(typeIndex)
        {}

        std::string Name();
        std::string PrettyName();
        size_t Hash();

    private:
        std::type_index mTypeIndex;
    };

    template <typename T>
    inline TypeInfo GetTypeInfo()
    {
        TypeInfo info(typeid(T));
        return info;
    }
}
