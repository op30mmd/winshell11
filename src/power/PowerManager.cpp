#include "PowerManager.h"
#include <windows.h>
#include <powrprof.h>
#include "common/Logger.h"

namespace shell::power {

void PowerManager::Lock() {
    LockWorkStation();
}

void PowerManager::SignOut() {
    ExitWindowsEx(EWX_LOGOFF, 0);
}

void PowerManager::Restart() {
    if (SetShutdownPrivilege()) {
        ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_OTHER);
    }
}

void PowerManager::Shutdown() {
    if (SetShutdownPrivilege()) {
        ExitWindowsEx(EWX_SHUTDOWN, SHTDN_REASON_MAJOR_OTHER);
    }
}

void PowerManager::Sleep() {
    SetSuspendState(FALSE, FALSE, FALSE);
}

bool PowerManager::SetShutdownPrivilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;

    LookupPrivilegeValue(nullptr, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, nullptr, nullptr);

    return GetLastError() == ERROR_SUCCESS;
}

} // namespace shell::power
