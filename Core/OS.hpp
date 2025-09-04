/*****************************************************************//**
 * \file   OS.hpp
 * \brief  Interface file for operating system functions
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include <filesystem>

namespace Gep
{
    // creates a saveas dialog box at the given filepath. The hint is english for the given extesion, such as "Object File" for ".obj"
    std::filesystem::path DialogBox_SaveAs(const std::filesystem::path& initialDir, const std::string& hint, const std::string& extension, const std::string& defaultFileName);

    // creates a file picker dialog box at the starting at the given filepath. The hint is english for the given extesion, such as "Object File" for ".obj"
    std::filesystem::path DialogBox_PickFile(const std::filesystem::path& initialDir, const std::string& hint, const std::string& extension);

    // create a folder picker dialog starting at the given filepath
    std::filesystem::path DialogBox_PickFolder(const std::filesystem::path& initialDir);

    // usually dlls only work next to the executable, this will change the folder that they are searched for in
    void SetDynamicLibraryDirectory(const std::filesystem::path& folder);

    // opens a given folderpath in explorer and opens a filepath in whatever the default editor is for the file
    void OpenInExplorer(const std::filesystem::path& path);

}
