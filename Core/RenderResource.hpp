/*****************************************************************//**
 * \file   RenderResource.hpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include <filesystem>
#include <set>

#include "Core.hpp"

#include "OpenGLRenderer.hpp"

#include "KeyedVector.hpp"
#include "Shader.hpp"

#include "Renderer.hpp"

namespace Gep
{
    class EngineManager;
}

namespace Client
{
    class RenderResource
    {
    public:
        Gep::OpenGLRenderer mRenderer;

        //Gep::OpenGLRenderer mOldRenderer;
        std::unique_ptr<Gep::Renderer> mNewRenderer;

    private:
        std::unordered_map<std::filesystem::path, size_t> mLoadedMeshes;
    };
}
