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

namespace gtl
{
    // uuid base class, can be used to create different types of uuids with different sizes and segmentations
    template <size_t SizeV, size_t SegmentsV>
    struct basic_uuid
    {
        static constexpr size_t size = SizeV;         // the total size in bytes of the uuid
        static constexpr size_t segments = SegmentsV; // the amount of segments to divide the uuid by when converting to a string
        std::array<uint8_t, size> bytes{};            // the bytes of the uuid

        // converts the uuid to a string, segments will be separated by "-"
        std::string to_string() const;

        // returns false if the uuid is all zeros, true otherwise
        bool is_valid() const;

        // converts to a string then outputs
        friend std::ostream& operator<<(std::ostream& os, const basic_uuid& uuid);

        // default comparison operators, compares the bytes of the uuid
        friend auto operator<=>(const basic_uuid&, const basic_uuid&) = default;
    };
}

// std::hash specialization to allow Gep::UUID in unordered containers
namespace std
{
    template <size_t Size, size_t Segments>
    struct hash<gtl::basic_uuid<Size, Segments>>
    {
        size_t operator()(const gtl::basic_uuid<Size, Segments>& id) const noexcept
        {
            const std::string_view sv{ reinterpret_cast<const char*>(id.bytes.data()), id.bytes.size() };
            return std::hash<std::string_view>{}(sv);
        }
    };
}

namespace gtl
{
    // a default uuid type that is usually more than enough for most use cases, it is 24 bytes total, divided into 3 segments of 8 bytes each
    using uuid = basic_uuid<24, 3>;
}

// functions for working with basic_uuids, such as generating and converting from string
namespace gtl
{
    // converts a string to a basic_uuid, "-" will be ignored.
    template <size_t SizeV = uuid::size, size_t SegmentsV = uuid::segments>
    basic_uuid<SizeV, SegmentsV> to_uuid(std::string string);

    // generates a new basic_uuid with random bytes
    template <size_t SizeV = uuid::size, size_t SegmentsV = uuid::segments>
    basic_uuid<SizeV, SegmentsV> generate_uuid();
}

#include "uuid.inl"
