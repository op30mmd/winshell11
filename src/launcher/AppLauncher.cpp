#include "AppLauncher.h"
#include "common/Logger.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>

#pragma comment(lib, "Shlwapi.lib")

namespace shell::launcher {

static std::wstring ExpandEnv(const std::wstring& path) {
    if (path.find(L'%') == std::wstring::npos)
        return path;
    wchar_t buf[MAX_PATH * 2];
    DWORD len = ExpandEnvironmentStringsW(path.c_str(), buf, MAX_PATH * 2);
    if (len > 0 && len <= MAX_PATH * 2)
        return buf;
    return path;
}

static std::wstring ResolveShortcut(const std::wstring& lnkPath) {
    IShellLinkW* psl = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
        IID_IShellLinkW, (void**)&psl);
    if (FAILED(hr)) return lnkPath;

    IPersistFile* ppf = nullptr;
    hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
    if (FAILED(hr)) { psl->Release(); return lnkPath; }

    hr = ppf->Load(lnkPath.c_str(), STGM_READ);
    ppf->Release();
    if (FAILED(hr)) { psl->Release(); return lnkPath; }

    // Get PIDL first for an expanded path
    LPITEMIDLIST pidl = nullptr;
    hr = psl->GetIDList(&pidl);
    if (SUCCEEDED(hr) && pidl) {
        wchar_t target[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, target)) {
            CoTaskMemFree(pidl);
            psl->Release();
            return target;
        }
        CoTaskMemFree(pidl);
    }

    // Fallback: raw path + env expansion
    wchar_t target[MAX_PATH];
    hr = psl->GetPath(target, MAX_PATH, nullptr, SLGP_RAWPATH);
    psl->Release();
    if (FAILED(hr) || target[0] == 0) return lnkPath;

    return ExpandEnv(target);
}

static AppType CategorizeTarget(const std::wstring& target, const std::wstring& name) {
    std::wstring lowerTarget;
    for (auto c : target) lowerTarget += towlower(c);

    std::wstring ext;
    size_t dot = lowerTarget.rfind(L'.');
    if (dot != std::wstring::npos)
        ext = lowerTarget.substr(dot);

    // Check by extension
    if (ext == L".cpl") return AppType::ControlPanel;
    if (ext == L".url") return AppType::WebLink;
    if (ext == L".msc") return AppType::SystemTool;

    if (ext == L".exe") {
        // Control panel / settings
        if (lowerTarget.find(L"control.exe") != std::wstring::npos)
            return AppType::ControlPanel;

        // System tools
        if (lowerTarget.find(L"\\system32\\") != std::wstring::npos ||
            lowerTarget.find(L"\\syswow64\\") != std::wstring::npos) {
            std::wstring fname = lowerTarget.substr(lowerTarget.rfind(L'\\') + 1);
            if (fname == L"mmc.exe" || fname == L"taskmgr.exe" ||
                fname == L"compmgmt.exe" || fname == L"devmgmt.exe" ||
                fname == L"diskmgmt.exe" || fname == L"eventvwr.exe" ||
                fname == L"gpedit.exe" || fname == L"lusrmgr.exe" ||
                fname == L"secpol.exe" || fname == L"perfmon.exe" ||
                fname == L"resmon.exe" || fname == L"msconfig.exe" ||
                fname == L"msinfo32.exe" || fname == L"dfrgui.exe" ||
                fname == L"cleanmgr.exe" || fname == L"regedit.exe" ||
                fname == L"services.msc" || fname == L"taskschd.msc" ||
                fname == L"wf.msc" || fname == L"printmanagement.msc" ||
                fname == L"secpol.msc" || fname == L"comexp.msc" ||
                fname == L"eventvwr.msc" || fname == L"perfmon.msc" ||
                fname == L"compmgmt.msc" || fname == L"lusrmgr.msc" ||
                fname == L"gpedit.msc" || fname == L"devmgmt.msc" ||
                fname == L"diskmgmt.msc" || fname == L"iscsicpl.exe" ||
                fname == L"mdsched.exe" || fname == L"odbcad32.exe" ||
                fname == L"recoverydrive.exe" || fname == L"mstsc.exe" ||
                fname == L"psr.exe" || fname == L"charmap.exe" ||
                fname == L"snippingtool.exe" || fname == L"windowsandbox.exe" ||
                fname == L"narrator.exe" || fname == L"magnify.exe" ||
                fname == L"osk.exe" || fname == L"livecaptions.exe" ||
                fname == L"voiceaccess.exe")
                return AppType::SystemTool;
        }
        return AppType::Application;
    }

    // Name-based fallback
    std::wstring lowerName;
    for (auto c : name) lowerName += towlower(c);
    if (lowerName.find(L"control panel") != std::wstring::npos)
        return AppType::ControlPanel;
    if (lowerName.find(L"settings") != std::wstring::npos)
        return AppType::ControlPanel;
    if (lowerName.find(L"device manager") != std::wstring::npos)
        return AppType::SystemTool;

    return AppType::Unknown;
}

static bool IsAppItem(const AppItem& item) {
    // Skip common non-app items based on name
    std::wstring lowerName;
    for (auto c : item.name) lowerName += towlower(c);

    // Uninstallers / setup helpers
    if (lowerName.find(L"uninstall") == 0) return false;
    if (lowerName.find(L"setup - ") != std::wstring::npos) return false;
    if (lowerName.find(L"add to archive") != std::wstring::npos) return false;
    if (lowerName.find(L"extract here") != std::wstring::npos) return false;
    if (lowerName.find(L"extract...") != std::wstring::npos) return false;
    if (lowerName.find(L"open as archive") != std::wstring::npos) return false;
    if (lowerName.find(L"run any program") != std::wstring::npos) return false;
    if (lowerName.find(L"run web browser") != std::wstring::npos) return false;

    // Documentation files
    if (lowerName.find(L"readme") == 0) return false;
    if (lowerName.find(L"read me") == 0) return false;
    if (lowerName.find(L"faq") == 0) return false;
    if (lowerName.find(L"release notes") == 0) return false;
    if (lowerName.find(L"what's new") == 0) return false;
    if (lowerName.find(L"what is new") == 0) return false;
    if (lowerName.find(L"copyright") == 0) return false;
    if (lowerName.find(L"license") == 0) return false;
    if (lowerName.find(L"copying") == 0) return false;
    if (lowerName.find(L"about ") == 0) return false;
    if (lowerName.find(L"check for updates") == 0) return false;

    // Skip items that look like file paths (greetings.txt, history.txt)
    if (lowerName.find(L".txt") != std::wstring::npos) return false;
    if (lowerName.find(L".htm") != std::wstring::npos) return false;
    if (lowerName.find(L".chm") != std::wstring::npos) return false;

    // Items whose target is just notepad/cmd (shell to file links)
    std::wstring lowerTarget;
    for (auto c : item.target) lowerTarget += towlower(c);
    if (lowerTarget.find(L"notepad.exe") != std::wstring::npos &&
        lowerName.find(L".txt") != std::wstring::npos) return false;

    // Keep non-Unknown items, filter out Unknown that are just lnk-to-self or help/docs
    if (item.type == (int)AppType::Unknown) {
        // Keep if target is a real .exe
        if (lowerTarget.find(L".exe") != std::wstring::npos)
            return true;
        return false;
    }

    return true;
}

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
            item.target = ResolveShortcut(item.path);
            item.type = (int)CategorizeTarget(item.target, item.name);

            if (!IsAppItem(item))
                continue;

            LOG_INFO("App: %ls -> %ls (type=%d)", item.name.c_str(), item.target.c_str(), item.type);
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
