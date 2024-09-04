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
	constexpr std::uint64_t MAX_ENTITIES = 65536; // the maximum amount of entities in the engine
	constexpr std::uint8_t MAX_COMPONETS = 32;    // the maximum amout of components that a singular entity can have

	using Signature   = std::bitset<MAX_COMPONETS>; // each bit represents a component that an entity may or may not have
	using Entity      = std::uint64_t;              // id representing an enity
	using ComponentBitPos = std::uint8_t;               // id representing a component

	template <typename num>
	constexpr num num_max() { return std::numeric_limits<num>().max(); };
}	