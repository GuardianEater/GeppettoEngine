/*****************************************************************//**
 * \file   uuid.inl
 * \brief  inline implementation for uuid related functions
 * 
 * \author 2018t
 * \date   February 2026
 *********************************************************************/

#pragma once
#include "uuid.hpp"
#include <random>

namespace gtl
{
    template<size_t SizeV, size_t SegmentsV>
    basic_uuid<SizeV, SegmentsV> to_uuid(std::string string)
    {
        basic_uuid<SizeV, SegmentsV> uuid{};

        // remove all '-' in the string
        string.erase(std::remove(string.begin(), string.end(), '-'), string.end());

        for (size_t i = 0; i < SizeV && i < string.size(); ++i)
            uuid.bytes[i] = string[i];

        return uuid;
    }

    template<size_t SizeV, size_t SegmentsV>
    basic_uuid<SizeV, SegmentsV> generate_uuid()
    {
        basic_uuid<SizeV, SegmentsV> uuid{};
        std::random_device rd;
        std::mt19937 gen(rd());

        // generate string from a set of all consanants and numbers because vowels are dangerous
        constexpr std::string_view charset = "BCDFGHJKLMNPQRSTVWXZbcdfghjklmnpqrstvwxz0123456789";
        std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);

        for (auto& byte : uuid.bytes)
        {
            byte = static_cast<char>(charset[dist(gen)]);
        }

        return uuid;
    }

    template<size_t SizeV, size_t SegmentsV>
    inline std::string basic_uuid<SizeV, SegmentsV>::to_string() const
    {
        constexpr size_t bytesPerSegment = SizeV / SegmentsV;

        std::string result;
        result.reserve(SizeV + SegmentsV);

        for (size_t i = 0; i < SizeV; ++i)
        {
            result.push_back(bytes[i]);

            if (i % bytesPerSegment == bytesPerSegment - 1)
                result.push_back('-');
        }
        result.pop_back();

        return result;
    }

    template<size_t SizeV, size_t SegmentsV>
    inline bool basic_uuid<SizeV, SegmentsV>::is_valid() const
    {
        return *this != basic_uuid<SizeV, SegmentsV>{};
    }

    template <size_t SizeV, size_t SegmentsV>
    std::ostream& operator<<(std::ostream& os, const basic_uuid<SizeV, SegmentsV>& uuid)
    {
        os << uuid.to_string();

        return os;
    }
}
