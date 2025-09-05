/*****************************************************************//**
 * \file   OS.cpp
 * \brief
 *
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#include "pch.hpp"
#include "OS.hpp"

#ifdef _WIN32

#include "Windows.h"
#include <shobjidl.h> 

namespace Gep
{

    static std::wstring StringToWString(const std::string& str)
    {
        return std::wstring(str.begin(), str.end());
    }

    std::filesystem::path DialogBox_SaveAs(const std::filesystem::path& initialDir, const std::string& hint, const std::string& extension, const std::string& defaultFileName)
    {
        std::wstring w_extension = StringToWString(extension);
        std::wstring w_defaultFileName = StringToWString(defaultFileName);
        std::wstring w_hint = StringToWString(hint);

        HRESULT hr;
        IFileSaveDialog* pFileSave = nullptr;
        std::wstring filePath;

        // Initialize COM
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) return L"";

        // Create the File Save Dialog object
        hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));
        if (FAILED(hr)) 
        {
            CoUninitialize();
            return L"";
        }

        // Set the default file extension and filter
        std::wstring filter = L"*." + w_extension;
        std::wstring filterHint = w_hint + L" (" + filter + L")";

        COMDLG_FILTERSPEC filterSpec[] = {
            { filterHint.c_str(), filter.c_str() },
            { L"All Files (*.*)", L"*.*" }
        };
        pFileSave->SetFileTypes(2, filterSpec);
        pFileSave->SetDefaultExtension(w_extension.c_str()); // Default extension

        // Set the default file name
        pFileSave->SetFileName(w_defaultFileName.c_str());

        // Set the initial directory
        IShellItem* pFolder = nullptr;
        hr = SHCreateItemFromParsingName(std::filesystem::absolute(initialDir).c_str(), nullptr, IID_PPV_ARGS(&pFolder));
        if (SUCCEEDED(hr))
        {
            pFileSave->SetFolder(pFolder);
            pFolder->Release();
        }

        // Show the save file dialog
        hr = pFileSave->Show(nullptr);
        if (SUCCEEDED(hr))
        {
            IShellItem* pItem;
            hr = pFileSave->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr))
                {
                    filePath = pszFilePath;
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }

        pFileSave->Release();
        CoUninitialize();

        return filePath;
    }

    std::filesystem::path DialogBox_PickFile(const std::filesystem::path& initialDir, const std::string& hint, const std::string& extension)
    {
        std::filesystem::path filePath;

        // Initialize COM in a single-threaded apartment.
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) return filePath;

        IFileDialog* pFileDialog = nullptr;
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&pFileDialog));
        if (SUCCEEDED(hr))
        {
            // If a valid start directory is provided, create an IShellItem and set it.
            if (!initialDir.empty())
            {
                IShellItem* psiStartFolder = nullptr;
                hr = SHCreateItemFromParsingName(std::filesystem::absolute(initialDir).c_str(), nullptr, IID_PPV_ARGS(&psiStartFolder));
                if (SUCCEEDED(hr))
                {
                    pFileDialog->SetFolder(psiStartFolder);
                    pFileDialog->SetDefaultFolder(psiStartFolder);
                    psiStartFolder->Release();
                }
            }

            // Build the file filter based on the extension
            std::wstring extFilter = L"*.*";
            std::wstring description = L"All Files (*.*)";
            std::wstring defaultExt = L"";

            if (!extension.empty())
            {
                std::wstring extW(extension.begin(), extension.end());
                if (extW.front() != L'*') extW = L"*." + extW;
                extFilter = extW;
                description = std::wstring(hint.begin(), hint.end()) + L" (*." + std::wstring(extension.begin(), extension.end()) + L")";
                defaultExt = std::wstring(extension.begin(), extension.end());
            }

            COMDLG_FILTERSPEC filterSpec[] = {
                { description.c_str(), extFilter.c_str() }
            };

            pFileDialog->SetFileTypes(1, filterSpec);
            if (!defaultExt.empty())
                pFileDialog->SetDefaultExtension(defaultExt.c_str());


            // Show the dialog.
            hr = pFileDialog->Show(NULL);
            if (SUCCEEDED(hr))
            {
                // Retrieve the selected file.
                IShellItem* pItem = nullptr;
                hr = pFileDialog->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath = nullptr;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                    if (SUCCEEDED(hr))
                    {
                        filePath = pszFilePath;
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileDialog->Release();
        }

        CoUninitialize();
        return filePath;
    }

    std::filesystem::path DialogBox_PickFolder(const std::filesystem::path& initialDir)
    {
        std::filesystem::path folderPath;

        // Initialize COM in a single-threaded apartment.
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) return folderPath;

        IFileDialog* pFileDialog = nullptr;
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog));
        if (SUCCEEDED(hr))
        {
            // Set options to enable folder picking.
            DWORD dwOptions = 0;
            hr = pFileDialog->GetOptions(&dwOptions);
            if (SUCCEEDED(hr))
            {
                hr = pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);
            }

            // If a valid start directory is provided, create an IShellItem and set it.
            if (!initialDir.empty() && SUCCEEDED(hr))
            {
                IShellItem* psiStartFolder = nullptr;
                hr = SHCreateItemFromParsingName(std::filesystem::absolute(initialDir).c_str(), nullptr, IID_PPV_ARGS(&psiStartFolder));
                if (SUCCEEDED(hr))
                {
                    pFileDialog->SetFolder(psiStartFolder);
                    pFileDialog->SetDefaultFolder(psiStartFolder);
                    psiStartFolder->Release();
                }
            }

            // Display the dialog.
            if (SUCCEEDED(hr))
            {
                hr = pFileDialog->Show(NULL);
                if (SUCCEEDED(hr))
                {
                    // Retrieve the selected folder.
                    IShellItem* pItem = nullptr;
                    hr = pFileDialog->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFolderPath = nullptr;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath);
                        if (SUCCEEDED(hr))
                        {
                            folderPath = pszFolderPath;
                            CoTaskMemFree(pszFolderPath);
                        }
                        pItem->Release();
                    }
                }
            }
            pFileDialog->Release();
        }

        CoUninitialize();
        return folderPath;
    }

    void SetDynamicLibraryDirectory(const std::filesystem::path& folder)
    {
        //SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        //DLL_DIRECTORY_COOKIE cookie = AddDllDirectory(folder.wstring().c_str());
    }

    void OpenInExplorer(const std::filesystem::path& path)
    {
        ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}
#else // ^^^ _WIN32 ^^^

namespace Gep
{
    std::filesystem::path DialogBox_SaveAs(const std::filesystem::path& initialDir, const std::string& hint, const std::string& extension, const std::string& defaultFileName)
    {
        throw "The function DialogBox_SaveAs does not have a definition on this OS";
    }

    std::filesystem::path DialogBox_PickFile(const std::filesystem::path& initialDir, const std::string& hint, const std::string& extension)
    {
        throw "The function DialogBox_PickFile does not have a definition on this OS";
    }

    std::filesystem::path DialogBox_PickFolder(const std::filesystem::path& initialDir)
    {
        throw "The function DialogBox_PickFolder does not have a definition on this OS";
    }

    void SetDynamicLibraryDirectory(const std::filesystem::path& folder)
    {
        throw "The function SetDynamicLibraryDirectory does not have a definition on this OS";
    }
} // namespace Gep

#endif // !_WIN32