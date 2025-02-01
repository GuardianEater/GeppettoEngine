/*****************************************************************//**
 * \file   pch.hpp
 * \brief  precompiled header
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#pragma once

// std
#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <optional>
#include <variant>
#include <tuple>
#include <any>
#include <span>
#include <typeindex>
#include <type_traits>
#include <concepts>
#include <ranges>
#include <numeric>
#include <iterator>
#include <filesystem>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <regex>
#include <random>
#include <fstream>
#include <iomanip>

// imgui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// glfw
#include <glew.h>
#include <glfw/glfw3.h>

// sol
#include "sol/sol.hpp"

// log
#include "Logger.hpp"

// reflect-cpp
#include "rfl.hpp"
#include "rfl/json.hpp"