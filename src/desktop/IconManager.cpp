#include "IconManager.h"
#include <shlobj.h>
#include <shlwapi.h>
#include "common/Logger.h"

#pragma comment(lib, "Shlwapi.lib")

namespace shell::desktop {

void IconManager::EnumerateIcons() {
    m_icons.clear();

    auto enumerateFromFolder = [this](REFKNOWNFOLDERID folderId) {
        PIDLIST_ABSOLUTE pidlFolder;
        if (SUCCEEDED(SHGetKnownFolderIDList(folderId, 0, nullptr, &pidlFolder))) {
            IShellFolder* pShellFolder;
            if (SUCCEEDED(SHBindToObject(nullptr, pidlFolder, nullptr, IID_PPV_ARGS(&pShellFolder)))) {
                IEnumIDList* pEnum;
                if (SUCCEEDED(pShellFolder->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum))) {
                    PITEMID_CHILD pidlChild;
                    while (pEnum->Next(1, &pidlChild, nullptr) == S_OK) {
                        STRRET strName;
                        if (SUCCEEDED(pShellFolder->GetDisplayNameOf(pidlChild, SIGDN_NORMALDISPLAY, &strName))) {
                            PWSTR pszName;
                            StrRetToStrW(&strName, pidlChild, &pszName);

                            DesktopIcon icon;
                            icon.name = pszName;

                            // Get File System Path
                            STRRET strPath;
                            if (SUCCEEDED(pShellFolder->GetDisplayNameOf(pidlChild, SIGDN_FILESYSPATH, &strPath))) {
                                PWSTR pszPath;
                                StrRetToStrW(&strPath, pidlChild, &pszPath);
                                icon.path = pszPath;
                                CoTaskMemFree(pszPath);
                            }

                            m_icons.push_back(icon);
                            CoTaskMemFree(pszName);
                        }
                        CoTaskMemFree(pidlChild);
                    }
                    pEnum->Release();
                }
                pShellFolder->Release();
            }
            CoTaskMemFree(pidlFolder);
        }
    };

    enumerateFromFolder(FOLDERID_Desktop);
    enumerateFromFolder(FOLDERID_PublicDesktop);
}

} // namespace shell::desktop
