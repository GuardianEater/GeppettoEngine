/*****************************************************************//**
 * \file   TypeID.cpp
 * \brief  implementation for the TypeID
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#include "pch.hpp"

#include "TypeID.hpp"

namespace Gep
{
    std::string TypeInfo::Name()
    {
        return mTypeIndex.name();
    }

    std::string TypeInfo::PrettyName()
    {
        // get the name of the type and remove the 'class ' or 'struct ' from the front
        std::string name = Name();
        if      (name.find("class ") != std::string::npos) name.erase(0, 6);
        else if (name.find("struct ") != std::string::npos) name.erase(0, 7);

        const size_t pos = name.rfind("::");
        if (pos != std::string::npos) name = name.substr(pos + 2); // +2 to skip "::"

        return name;
    }

    size_t TypeInfo::Hash()
    {
        return mTypeIndex.hash_code();
    }
}
