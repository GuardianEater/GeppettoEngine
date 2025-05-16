/*****************************************************************//**
 * \file   Core.hpp
 * \brief  Contains a bunch of useful things that are needed in almost every file
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

 // standard ///////////////////////////////////////////////////////////////////

#include <vector>
#include <list>
#include <array>
#include <deque>

#include <set>
#include <unordered_set>

#include <map>
#include <unordered_map>

#include <queue>
#include <stack>

#include <memory>
#include <bitset>

#include <string>
#include <iostream>

#include <fstream>
#include <filesystem>

#include <typeinfo>
#include <type_traits>
#include <typeindex>

#include <chrono>
#include <limits>
#include <numeric>
#include <functional>

#include <assert.h>
#include <cassert>

// API Control ////////////////////////////////////////////////////////////////

// engine /////////////////////////////////////////////////////////////////////

namespace Gep
{
    template <typename num>
    constexpr num num_max() { return std::numeric_limits<num>().max(); };

    constexpr uint64_t MAX_ENTITIES = 65536; // the maximum amount of entities in the engine
    constexpr uint8_t MAX_COMPONENTS = 64;    // the maximum amout of components that a singular entity can have
    constexpr size_t INVALID_ENTITY = num_max<size_t>();     // the id of an entity that is not valid
    
    using Signature = std::bitset<MAX_COMPONENTS>; // each bit represents a component that an entity may or may not have
    using Entity = uint64_t;              // id representing an enity
    using ComponentBitPos = uint8_t;               // id representing a component

    // created using the static member functions.
    class UUID
    {
    private:
        static constexpr size_t size = 24;
        static constexpr size_t segments = 3; // determines the amount of dashes to put in it
        static constexpr size_t bytesPerSegment = size / segments;

        std::array<uint8_t, size> bytes{};

    public:
        const std::array<uint8_t, size>& GetBytes() { return bytes; };

        static UUID FromString(std::string string);
        static UUID GenerateNew();

        std::string ToString() const;

        bool IsValid() const;

        friend std::ostream& operator<<(std::ostream& os, const UUID& uuid);
        friend auto operator<=>(const UUID&, const UUID&) = default;
    };
}