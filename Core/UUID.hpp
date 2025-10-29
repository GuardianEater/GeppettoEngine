/*****************************************************************//**
 * \file   UUID.hpp
 * \brief  generates a universally unique id
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   October 2025
 *********************************************************************/

#pragma once

#include <array>
#include <string>

namespace Gep
{
    // created using the static member functions.
    class UUID
    {
    private:
        static constexpr size_t size = 24;
        static constexpr size_t segments = 3; // determines the amount of dashes to put in it when interpretted as a string
        static constexpr size_t bytesPerSegment = size / segments;

        std::array<uint8_t, size> bytes{};

    public:
        const std::array<uint8_t, size>& GetBytes() const { return bytes; };

        static UUID FromString(std::string string);
        static UUID GenerateNew();

        std::string ToString() const;

        bool IsValid() const;

        friend std::ostream& operator<<(std::ostream& os, const UUID& uuid);
        friend auto operator<=>(const UUID&, const UUID&) = default;
    };
}

// std::hash specialization to allow Gep::UUID in unordered containers
namespace std
{
    template <>
    struct hash<Gep::UUID>
    {
        size_t operator()(const Gep::UUID& id) const noexcept
        {
            const auto& bytes = id.GetBytes();
            const std::string_view sv{ reinterpret_cast<const char*>(bytes.data()), bytes.size() };
            return std::hash<std::string_view>{}(sv);
        }
    };
}