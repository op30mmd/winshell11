#include "IconManager.h"
#include "common/Logger.h"
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

namespace shell::desktop {

void IconManager::EnumerateIcons() {
    m_icons.clear();

    int gridSpacingX = 100;
    int gridSpacingY = 96;
    int marginX = 12;
    int marginY = 12;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int iconsPerRow = (screenW - marginX * 2) / gridSpacingX;
    if (iconsPerRow < 1)
        iconsPerRow = 1;

    int col = 0, row = 0;

    // Enumerate from the virtual desktop folder — this includes both
    // filesystem items (files/folders on desktop) and virtual items
    // (Recycle Bin, This PC, Network, etc.)
    IShellFolder* pDesktopFolder = nullptr;
    if (FAILED(SHGetDesktopFolder(&pDesktopFolder)))
        return;

    // Get the desktop PIDL for constructing absolute PIDLs
    PIDLIST_ABSOLUTE pidlDesktop = nullptr;
    SHGetKnownFolderIDList(FOLDERID_Desktop, 0, nullptr, &pidlDesktop);

    IEnumIDList* pEnum = nullptr;
    HRESULT hr = pDesktopFolder->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &pEnum);
    if (SUCCEEDED(hr) && pEnum) {
        PITEMID_CHILD pidlChild;
        while (pEnum->Next(1, &pidlChild, nullptr) == S_OK) {
            STRRET strName;
            if (SUCCEEDED(pDesktopFolder->GetDisplayNameOf(pidlChild, SIGDN_NORMALDISPLAY, &strName))) {
                PWSTR pszName;
                StrRetToStrW(&strName, pidlChild, &pszName);

                // Skip empty names
                if (pszName && wcslen(pszName) > 0) {
                    DesktopIcon icon;
                    icon.name = pszName;
                    icon.x = marginX + col * gridSpacingX;
                    icon.y = marginY + row * gridSpacingY;

                    // Build absolute PIDL
                    icon.pidl = ILCombine(pidlDesktop, pidlChild);

                    // Get filesystem path (may fail for virtual items)
                    STRRET strPath;
                    if (SUCCEEDED(pDesktopFolder->GetDisplayNameOf(pidlChild, (SHGDNF)SIGDN_FILESYSPATH, &strPath))) {
                        PWSTR pszPath;
                        StrRetToStrW(&strPath, pidlChild, &pszPath);
                        icon.path = pszPath;
                        CoTaskMemFree(pszPath);
                    }

                    // Get the parsing name (works for all items including virtual)
                    STRRET strParsing;
                    if (SUCCEEDED(pDesktopFolder->GetDisplayNameOf(pidlChild, (SHGDNF)SIGDN_DESKTOPABSOLUTEPARSING, &strParsing))) {
                        PWSTR pszParsing;
                        StrRetToStrW(&strParsing, pidlChild, &pszParsing);
                        icon.parsingName = pszParsing;
                        CoTaskMemFree(pszParsing);
                    }

                    m_icons.push_back(icon);

                    col++;
                    if (col >= iconsPerRow) {
                        col = 0;
                        row++;
                    }
                }

                CoTaskMemFree(pszName);
            }
            CoTaskMemFree(pidlChild);
        }
        pEnum->Release();
    }

    if (pidlDesktop)
        CoTaskMemFree(pidlDesktop);
    pDesktopFolder->Release();
}

void IconManager::FreeIcons() {
    for (auto& icon : m_icons) {
        if (icon.pidl) {
            CoTaskMemFree(icon.pidl);
            icon.pidl = nullptr;
        }
    }
    m_icons.clear();
}

} // namespace shell::desktop
