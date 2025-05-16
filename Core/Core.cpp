/*****************************************************************//**
 * \file   Core.cpp
 * \brief  pch file
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   January 2025
 *********************************************************************/

#include "pch.hpp"

#include "Core.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Gep
{
    UUID UUID::FromString(std::string str)
    {
        UUID uuid{};

        str.erase(std::remove(str.begin(), str.end(), '-'), str.end());

        size_t index = 0;
        for (auto c : str)
        {
            uuid.bytes[index] = c;
            ++index;
        }

        return uuid;
    }

    UUID UUID::GenerateNew()
    {
        UUID uuid{};
        std::random_device rd;
        std::mt19937 gen(rd());

        constexpr std::string_view charset = "BCDFGHJKLMNPQRSTVWXZbcdfghjklmnpqrstvwxz0123456789";
        std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);

        for (auto& byte : uuid.bytes)
        {
            byte = static_cast<char>(charset[dist(gen)]);
        }

        return uuid;
    }

    std::string UUID::ToString() const
    {
        std::string result;
        result.reserve(size + segments);

        for (size_t i = 0; i < size; ++i)
        {
            result.push_back(bytes[i]);

            if (i % bytesPerSegment == bytesPerSegment - 1)
                result.push_back('-');
        }
        result.pop_back();

        return result;
    }

    bool UUID::IsValid() const
    {
        return *this != UUID{};
    }

    std::ostream& operator<<(std::ostream& os, const UUID& uuid)
    {
        os << uuid.ToString();

        return os;
    }
}

