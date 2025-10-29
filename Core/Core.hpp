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

    enum class EngineState
    {
        Play,  // the state when the game is playing
        Pause, // the state when the game is paused
        Edit,  // the state when in edit mode
        None,  // it is only this state when the engine is starting and when the engine is endings
    };
}