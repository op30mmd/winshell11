#include "AppLauncher.h"
#include "common/Logger.h"
#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

namespace shell::launcher {

static void EnumerateFolder(const std::wstring& path, std::vector<AppItem>& apps) {
    std::wstring search = path + L"\\*.lnk";
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            AppItem item;
            item.name = ffd.cFileName;
            if (item.name.size() > 4)
                item.name.resize(item.name.size() - 4);
            item.path = path + L"\\" + ffd.cFileName;
            apps.push_back(std::move(item));
        }
    } while (FindNextFileW(hFind, &ffd) != 0);
    FindClose(hFind);

    search = path + L"\\*";
    hFind = FindFirstFileW(search.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0) {
                EnumerateFolder(path + L"\\" + ffd.cFileName, apps);
            }
        }
    } while (FindNextFileW(hFind, &ffd) != 0);
    FindClose(hFind);
}

void AppLauncher::EnumerateApps() {
    m_apps.clear();

    PWSTR path;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Programs, 0, nullptr, &path))) {
        EnumerateFolder(path, m_apps);
        CoTaskMemFree(path);
    }
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_CommonPrograms, 0, nullptr, &path))) {
        EnumerateFolder(path, m_apps);
        CoTaskMemFree(path);
    }
}

void AppLauncher::Launch(const AppItem& item) {
    ShellExecuteW(nullptr, L"open", item.path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

bool AppLauncher::ShowMenu(HWND hParent) {
    if (m_apps.empty())
        return false;

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu)
        return false;

    for (size_t i = 0; i < m_apps.size() && i < 100; i++) {
        MENUITEMINFOW mii = {sizeof(mii)};
        mii.fMask = MIIM_STRING | MIIM_ID;
        mii.wID = (UINT)(i + 1);
        mii.dwTypeData = (LPWSTR)m_apps[i].name.c_str();
        InsertMenuItemW(hMenu, (UINT)i, TRUE, &mii);
    }

    RECT taskbarRc;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &taskbarRc, 0);
    POINT pt = {0, taskbarRc.top};

    UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, hParent, nullptr);
    DestroyMenu(hMenu);

    if (cmd > 0 && cmd <= (UINT)m_apps.size()) {
        Launch(m_apps[cmd - 1]);
        return true;
    }
    return false;
}

} // namespace shell::launcher
