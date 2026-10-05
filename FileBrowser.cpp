#include "FileBrowser.h"
#include <shobjidl.h>
#include <windows.h>

std::string select_directory() {
    std::string path;
    IFileDialog *pfd = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }
        if (SUCCEEDED(pfd->Show(NULL))) {
            IShellItem *psi;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszPath;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    std::wstring ws(pszPath);
                    if (!ws.empty()) {
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), NULL, 0, NULL, NULL);
                        path = std::string(size_needed, 0);
                        WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), &path[0], size_needed, NULL, NULL);
                    }
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return path;
}

std::string select_archive() {
    std::string path;
    IFileDialog *pfd = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
            pfd->SetOptions(dwOptions | FOS_FORCEFILESYSTEM);
        }
        COMDLG_FILTERSPEC rgSpec[] = { { L"Archives", L"*.zip;*.rar;*.7z" } };
        pfd->SetFileTypes(1, rgSpec);
        if (SUCCEEDED(pfd->Show(NULL))) {
            IShellItem *psi;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszPath;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    std::wstring ws(pszPath);
                    if (!ws.empty()) {
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), NULL, 0, NULL, NULL);
                        path = std::string(size_needed, 0);
                        WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), &path[0], size_needed, NULL, NULL);
                    }
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return path;
}

std::vector<std::string> select_directories() {
    std::vector<std::string> paths;
    IFileOpenDialog *pfd = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT);
        }
        if (SUCCEEDED(pfd->Show(NULL))) {
            IShellItemArray *psiaResult;
            if (SUCCEEDED(pfd->GetResults(&psiaResult))) {
                DWORD count;
                psiaResult->GetCount(&count);
                for (DWORD i = 0; i < count; i++) {
                    IShellItem *psi;
                    if (SUCCEEDED(psiaResult->GetItemAt(i, &psi))) {
                        PWSTR pszPath;
                        if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                            std::wstring ws(pszPath);
                            if (!ws.empty()) {
                                int size_needed = WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), NULL, 0, NULL, NULL);
                                std::string path(size_needed, 0);
                                WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), &path[0], size_needed, NULL, NULL);
                                paths.push_back(path);
                            }
                            CoTaskMemFree(pszPath);
                        }
                        psi->Release();
                    }
                }
                psiaResult->Release();
            }
        }
        pfd->Release();
    }
    return paths;
}

std::vector<std::string> select_archives() {
    std::vector<std::string> paths;
    IFileOpenDialog *pfd = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
            pfd->SetOptions(dwOptions | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT);
        }
        COMDLG_FILTERSPEC rgSpec[] = { { L"Archives", L"*.zip;*.rar;*.7z" } };
        pfd->SetFileTypes(1, rgSpec);
        if (SUCCEEDED(pfd->Show(NULL))) {
            IShellItemArray *psiaResult;
            if (SUCCEEDED(pfd->GetResults(&psiaResult))) {
                DWORD count;
                psiaResult->GetCount(&count);
                for (DWORD i = 0; i < count; i++) {
                    IShellItem *psi;
                    if (SUCCEEDED(psiaResult->GetItemAt(i, &psi))) {
                        PWSTR pszPath;
                        if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                            std::wstring ws(pszPath);
                            if (!ws.empty()) {
                                int size_needed = WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), NULL, 0, NULL, NULL);
                                std::string path(size_needed, 0);
                                WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), &path[0], size_needed, NULL, NULL);
                                paths.push_back(path);
                            }
                            CoTaskMemFree(pszPath);
                        }
                        psi->Release();
                    }
                }
                psiaResult->Release();
            }
        }
        pfd->Release();
    }
    return paths;
}
