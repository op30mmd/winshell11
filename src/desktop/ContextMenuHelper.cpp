#include "DesktopWindow.h"
#include "common/Logger.h"
#include <shlobj.h>

namespace shell::desktop {

// Example of how to show the native context menu for a set of shell items
void ShowContextMenu(HWND hwnd, IShellFolder* parentFolder, const std::vector<PITEMID_CHILD>& items, POINT pt) {
    IContextMenu* pContextMenu = nullptr;
    HRESULT hr = parentFolder->GetUIObjectOf(
        hwnd,
        (UINT)items.size(),
        items.data(),
        IID_IContextMenu,
        nullptr,
        (void**)&pContextMenu
    );

    if (SUCCEEDED(hr)) {
        HMENU hMenu = CreatePopupMenu();
        if (hMenu) {
            hr = pContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_NORMAL);
            if (SUCCEEDED(hr)) {
                UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
                if (cmd > 0) {
                    CMINVOKECOMMANDINFO ici = { sizeof(ici) };
                    ici.lpVerb = MAKEINTRESOURCEA(cmd - 1);
                    ici.nShow = SW_SHOWNORMAL;
                    pContextMenu->InvokeCommand(&ici);
                }
            }
            DestroyMenu(hMenu);
        }
        pContextMenu->Release();
    }
}

// WindowProc would call this on WM_CONTEXTMENU
} // namespace shell::desktop
