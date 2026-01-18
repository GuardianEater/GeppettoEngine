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

#ifdef _DEBUG
    // in debug mode will evaluate as usual
    #define debug_if(x) if(x)
#else
    // in release mode will get optimized out
    #define debug_if(x) if (false)
#endif

namespace Gep
{
    template <typename NumType>
    constexpr NumType NumMax() { return std::numeric_limits<NumType>().max(); };

    using Entity          = uint64_t; // id representing an entity
    using ComponentBitPos = uint8_t;  // id representing a component

    constexpr uint64_t MAX_ENTITIES  = 65536;            // the maximum amount of entities that can exist at once in the engine
    constexpr uint8_t MAX_COMPONENTS = 64;               // the maximum amount of components that a singular entity can have
    constexpr Entity INVALID_ENTITY  = NumMax<Entity>(); // the id of an entity that is not valid
    
    using Signature = std::bitset<MAX_COMPONENTS>; // each bit represents a component that an entity may or may not have

    enum class EngineState
    {
        None,  // it is only this state when the engine is starting and when the engine is endings
        Play,  // the state when the game is playing
        Pause, // the state when the game is paused
        Edit,  // the state when in edit mode
    };
}