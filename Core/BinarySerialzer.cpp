/*****************************************************************//**
 * \file   BinarySerialzer.cpp
 * \brief  serialization of binary data
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"

#include "BinarySerializer.hpp"

namespace Gep
{
    Serializer::Mode Serializer::mMode = Serializer::Mode::None;
    std::ostream* Serializer::mOutStream = nullptr;
    std::istream* Serializer::mInStream = nullptr;

}; // namespace Gep
