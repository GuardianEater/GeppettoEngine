/*****************************************************************//**
 * \file   Material.cpp
 * \brief  
 * 
 * \author 2018t
 * \date   March 2026
 *********************************************************************/

#include "pch.hpp"

#include "Material.hpp"

#include <Windows.h>

namespace OS
{
    static HICON GetIcon(const std::filesystem::path& path)
    {
        SHFILEINFO sfi{};
        if (SHGetFileInfo(path.wstring().c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON))
        {
            return sfi.hIcon;
        }

        return nullptr;
    }

    static Gep::Texture BitmapToTexture(HBITMAP bitmap)
    {
        if (!bitmap) return {};

        BITMAP bm{};
        if (!GetObject(bitmap, sizeof(bm), &bm))
        {
            Gep::Log::Error("Failed to convert bitmap to texture invalid bitmap handle");
            return {};
        }

        BITMAPINFO bmpInfo{};
        bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmpInfo.bmiHeader.biWidth = bm.bmWidth;
        bmpInfo.bmiHeader.biHeight = -bm.bmHeight;
        bmpInfo.bmiHeader.biPlanes = 1;
        bmpInfo.bmiHeader.biBitCount = 32;
        bmpInfo.bmiHeader.biCompression = BI_RGB;

        std::vector<BYTE> pixels(bm.bmWidth * bm.bmHeight * 4);
        HDC dc = GetDC(nullptr);
        int rows = GetDIBits(dc, bitmap, 0, bm.bmHeight, pixels.data(), &bmpInfo, DIB_RGB_COLORS);
        ReleaseDC(nullptr, dc);
        if (rows == 0)
        {
            Gep::Log::Error("Failed to convert bitmap to texture no rows");
            return {};
        }

        Gep::Texture texture;
        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.bmWidth, bm.bmHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        texture.handle = glGetTextureHandleARB(texture.id);
        glMakeTextureHandleResidentARB(texture.handle);

        DeleteObject(bitmap);

        return texture;
    }

    static Gep::Texture IconToTexture(HICON icon)
    {
        if (!icon) return {};

        ICONINFO iconInfo;
        if (!GetIconInfo(icon, &iconInfo)) return {};

        return BitmapToTexture(iconInfo.hbmColor);
    }
}

namespace Gep
{
    Texture Texture::LoadFileIcon(const std::filesystem::path& path)
    {
        HICON icon = OS::GetIcon(path);
        if (!icon)
        {
            Gep::Log::Error("Failed to load icon: [", path.string(), "]");
            return {};
        }

        Texture texture = OS::IconToTexture(icon);
        if (texture.id == 0)
        {
            Gep::Log::Error("Failed to convert icon to texture: [", path.string(), "]");
            return {};
        }

        return texture;
    }

    Texture Texture::Load(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            Gep::Log::Error("Cannot load texture: [", path.string(), "] does not exist");
            return {};
        }

        int width, height, channels;
        int required_channels = 4; // Force RGBA
        uint8_t* image = stbi_load(path.string().c_str(), &width, &height, &channels, required_channels);
        if (!image)
        {
            Gep::Log::Error("Failed to load texture: [", path.string(), "]");
            return {};
        }

        Texture tex = Texture::LoadFromPixels(image, width, height, required_channels);
        stbi_image_free(image);

        return tex;
    }

    Texture Texture::LoadFromPixels(const uint8_t* pixelData, size_t width, size_t height, int requiredChannels)
    {
        Texture texture{};
        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Ensure proper alignment
        GLenum iformat = (requiredChannels == 4) ? GL_RGBA8 : GL_RGB8;
        GLenum format = (requiredChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, iformat, width, height, 0, format, GL_UNSIGNED_BYTE, pixelData);

        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        texture.handle = glGetTextureHandleARB(texture.id);
        glMakeTextureHandleResidentARB(texture.handle);

        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture

        return texture;
    }

    Texture Texture::LoadFromMemory(const uint8_t* imageFileData, size_t size)
    {
        int requiredChannels = 4; // Force RGBA
        int width, height, channels;
        uint8_t* image = stbi_load_from_memory(imageFileData, size, &width, &height, &channels, requiredChannels);
        if (!image)
        {
            Gep::Log::Error("Failed to load texture from raw data");
            return{};
        }

        Texture tex = Texture::LoadFromPixels(image, width, height, requiredChannels);
        stbi_image_free(image);

        return tex;
    }

    Texture Texture::LoadHDR(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            Gep::Log::Error("Cannot load texture: [", path.string(), "] does not exist");
            return {};
        }

        int required_channels = 3;
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        float* image = stbi_loadf(path.string().c_str(), &width, &height, &channels, required_channels);
        stbi_set_flip_vertically_on_load(false);

        if (!image)
        {
            Gep::Log::Error("Failed to load hdr texture: [", path.string(), "]");
            return {};
        }

        Texture tex = Texture::LoadFromPixelsHDR(image, width, height, required_channels);
        stbi_image_free(image);

        return tex;
    }

    Texture Texture::LoadFromPixelsHDR(const float* pixelData, size_t width, size_t height, int requiredChannels)
    {
        Texture texture;
        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);

        GLenum iformat = (requiredChannels == 4) ? GL_RGBA16F : GL_RGB16F;
        GLenum format = (requiredChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, iformat, width, height, 0, format, GL_FLOAT, pixelData);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        texture.handle = glGetTextureHandleARB(texture.id);
        glMakeTextureHandleResidentARB(texture.handle);

        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture    }

        return texture;
    }
}

