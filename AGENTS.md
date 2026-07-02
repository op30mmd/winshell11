# AGENTS.md: CustomShell — Win32 Desktop Shell Project

## 🤖 Agent Persona & Objective
**Role:** You are an AI software engineering agent assisting with a Win32/WinUI 3 hybrid codebase focused on custom desktop shell development.
**Objective:** Analyze, debug, and implement shell features while adhering to the project's structure, abstractions, and conventions.
**Mindset:** Desktop shells have unique requirements for Z-order, workspace management, and system integration. The frontend is WinUI 3 (C#) and the backend is C++ (`shell_backend.dll`). Analyze the project's architecture before making changes.

---

## 📂 1. Project Architecture Overview

### Component Layout
The shell is split into two parts:

**WinUI 3 Frontend (`src/WinUIShell/`)**
- `App.xaml.cs` — Application entry. Creates 4 windows: Desktop, StartMenu (popup), Flyout (popup), Taskbar.
- `StartMenuControl.xaml/cs` — WinUI UserControl for the start menu (app list with icons, search). Uses `SB_Launcher_Enumerate`.
- `FlyoutControl.xaml/cs` — Quick settings panel (WiFi/BT toggles, volume, brightness, power buttons). Reads backend state.
- `IconHelper.cs` — Converts HICON → `SoftwareBitmapSource` via `System.Drawing` + `Windows.Graphics.Imaging`.
- `NativeMethods.cs` — All P/Invoke declarations for shell_backend.dll and Win32 APIs.
- `AppItemModel.cs` — Data model for app items with `Name`, `Path`, `Target`, `Type` (enum), `Icon`.

**C++ Backend (`src/backend/`, `src/common/`, `src/launcher/`, `src/desktop/`, etc.)**
- `exports.h/cpp` — Exported `extern "C"` functions called from C# via `[DllImport]`.
- `AppLauncher` — Enumerates Start Menu shortcuts, resolves `.lnk` targets via `IShellLink`, categorizes apps (Application/ControlPanel/SystemTool/WebLink).
- `IconManager` — Enumerates desktop icons via `SHGetDesktopFolder`, computes grid positions.
- `shell_backend.dll` — Single DLL loaded by WinUIShell.exe.

### Architecture Diagram
```
WinUIShell.exe (C#, WinUI 3)
  ├── Desktop Window (full-screen, wallpaper + icons via Canvas)
  ├── Start Menu (popup, app list with icons + search)
  ├── Flyout (popup, quick settings with real backend state)
  └── Taskbar (AppBar at bottom)
        └── shell_backend.dll (C++ P/Invoke)
              ├── AppLauncher (Start Menu .lnk scanner)
              ├── IconManager (desktop item enumeration)
              ├── PowerManager (lock/sleep/restart/shutdown)
              ├── Volume/Brightness (waveOut/WMI)
              ├── WiFi (WlanAPI) + Bluetooth (BluetoothAPI)
              └── Network (iphlpapi)
```

---

## 📐 2. Build System & Toolchain

### C++ Backend
- **Build:** `cmake -B build && cmake --build build --config Release`
- **Compiler:** MSVC with `/W4 /WX /analyze`
- **Output:** `build/src/backend/Release/shell_backend.dll`
- **After build, copy DLL:** `Copy-Item build/src/backend/Release/shell_backend.dll src/WinUIShell/bin/Debug/net8.0-windows10.0.19041.0/win-x64/shell_backend.dll -Force`

### WinUI 3 Frontend
- **Build:** `dotnet build src/WinUIShell/WinUIShell.csproj`
- **Framework:** `net8.0-windows10.0.19041.0`, Windows App SDK 1.6
- **Self-contained:** `WindowsAppSDKSelfContained=true` with `RuntimeIdentifier=win-x64`
- **NuGet deps:** `Microsoft.WindowsAppSDK 1.6.240923002`, `System.Drawing.Common 8.0.0`

### CI
- `.github/workflows/ci.yml` — GitHub Actions: CMake → C++ build → dotnet restore → dotnet build → Inno Setup (optional) → artifact upload.

### Known XAML Compiler Bug
The EXE `XamlCompiler.exe` silently exits with code 1 on XAML errors (WinAppSDK 1.6 bug). To see real errors:
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe" WinUIShell.csproj /p:UseXamlCompilerExecutable=false
```

---

## 🖥️ 3. Shell Design Rules

### A. App Enumeration & Categorization
- Apps are enumerated from `FOLDERID_Programs` + `FOLDERID_CommonPrograms` by scanning `.lnk` files.
- Each `.lnk` is resolved via `IShellLink::GetIDList` → `SHGetPathFromIDListW` for proper path expansion (environment variables like `%windir%` are expanded).
- Apps are categorized via `CategorizeTarget()`:
  - `.cpl` → `ControlPanel`
  - `.url` → `WebLink`
  - `.msc` → `SystemTool`
  - `.exe` → checked against known system tool list
  - Name-based fallback for Control Panel, Settings, Device Manager
- Non-app items are filtered: uninstallers, setup helpers, context menu shortcuts (`Add to archive...`, `Extract here...`), documentation files (`readme`, `faq`, `license`, `copyright`, `.txt`, `.htm`, `.chm`).

### B. SB_AppItem Struct
```cpp
typedef struct {
    wchar_t name[256];
    wchar_t path[1024];    // .lnk path
    wchar_t target[1024];  // resolved target (expanded)
    int type;              // 0=App, 1=ControlPanel, 2=SystemTool, 3=WebLink, 4=Unknown
} SB_AppItem;
```
Must match C# `[StructLayout]` exactly. When adding fields, update both sides.

### C. Icon Loading
- Use `IconHelper.FromPathAsync(path)` for converting shell icons to `SoftwareBitmapSource`.
- The function tries `SHGetFileInfoW` WITHOUT `SHGFI_USEFILEATTRIBUTES` first (resolves `.lnk` targets).
- Falls back to `SHGFI_USEFILEATTRIBUTES` for virtual/CLSID paths.
- For Start Menu, use the resolved `target` path (not the `.lnk` path) for icon extraction.
- NOTE: `AppItemModel` does NOT implement `INotifyPropertyChanged`. Icons must be set before adding items to the `ObservableCollection`.

### D. Flyout / Quick Panel
- Reads real backend state on load:
  - `SB_WiFi_IsEnabled()` / `SB_Bluetooth_IsEnabled()` for toggle state
  - `SB_Volume_Get()` / `SB_Brightness_Get()` for sliders
  - `SB_Brightness_HasControl()` for brightness visibility
  - `SB_Network_GetInfo()` for network info
- Toggle buttons show active (blue tint) / inactive state via background color change.

### E. WiFi & Bluetooth Backend
- **WiFi state:** `WlanQueryInterface` with `wlan_intf_opcode_current_operation_mode` — non-zero = enabled.
- **Bluetooth state:** `BluetoothFindFirstRadio` to detect hardware, then registry `HKLM\...\BTHPORT\Parameters\IsEnabled` for power state.
- Toggle functions: `WlanSetInterface` (WiFi), `ShellExecute ms-settings:bluetooth` (BT).

### F. Window Styles & Z-Order (C# side)
- **Desktop:** `WS_POPUP | WS_VISIBLE`, `WS_EX_TOOLWINDOW`. Set via `SetWindowLongW`. Positioned at `HWND_BOTTOM`.
- **Taskbar:** `WS_POPUP | WS_VISIBLE`, `WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE`. AppBar registered via `SHAppBarMessage`.
- **Popups (Start Menu, Flyout):** `WS_EX_NOACTIVATE` to avoid focus steal. Positioned relative to taskbar.

### G. AppBar Registration
```csharp
var abd = new APPBARDATA();
abd.cbSize = Marshal.SizeOf<APPBARDATA>();
abd.hWnd = hwnd;
abd.uEdge = NativeMethods.ABE_BOTTOM;
NativeMethods.SHAppBarMessage(NativeMethods.ABM_NEW, ref abd);
// Then ABM_SETPOS, etc.
```

---

## ⚠️ 4. Common Pitfalls

| Pitfall | Solution |
|---------|----------|
| XAML compiler doesn't show errors | Use VS MSBuild with `UseXamlCompilerExecutable=false` |
| Struct layout mismatch C++ ↔ C# | Verify `[StructLayout]`, `MarshalAs(UnmanagedType.ByValTStr, SizeConst=N)`, and field ordering match exactly |
| Icons not updating in ListView | Set `model.Icon` BEFORE adding to ObservableCollection (no INotifyPropertyChanged) |
| WiFi/BT toggle doesn't reflect state | Call `UpdateWifiState()`/`UpdateBtState()` after toggling |
| Shortcut target has `%windir%` unexpanded | Use `IShellLink::GetIDList` → `SHGetPathFromIDListW` for proper expansion |
| Non-app items in start menu | Add name filter in `IsAppItem()` in `AppLauncher.cpp` |
| DLL not updated when running | Manually copy `shell_backend.dll` to the WinUI output directory after C++ build |
| `SoftwareBitmap.LoadAsync` doesn't exist | Use `BitmapDecoder.CreateAsync(stream)` → `GetSoftwareBitmapAsync()` |
| `Color` / `Colors` not found | `using Windows.UI;` for `Color`, `using Microsoft.UI;` for `Colors` |

---

## 🔍 5. Debugging

### C++ Backend Logs
- Backend logs via `LOG_INFO("fmt", args)` — output to `OutputDebugStringA` and stdout.
- Read logs from the shell (stdout is captured when running from CLI) or use DebugView.

### C# App Diagnostics
- Use `System.Diagnostics.Debug.WriteLine()` or capture OutputDebugString from the WinUI app.
- Check if shell_backend.dll loads: look for "IconManager: found N icons" in output.

### Testing Runtime
```powershell
$exe = "src/WinUIShell/bin/Debug/net8.0-windows10.0.19041.0/win-x64/WinUIShell.exe"
$p = [System.Diagnostics.Process]::Start($exe)
Start-Sleep -Seconds 3
if (!$p.HasExited) { $p.Kill(); Write-Host "OK" }
```

---

## 🔧 6. Installer & Shell Registration

### Installer
- **Tool:** Inno Setup (`installer/CustomShell.iss`)
- Deploys `WinUIShell.exe` + all files from the release output directory.
- Runs `Enable-ShellLauncher.ps1` to register as boot shell.

### Shell Launcher v2
- Already configured: `config/ShellLauncherConfiguration.xml` + `config/Enable-ShellLauncher.ps1`
- Uses Windows `Client-EmbeddedShellLauncher` optional feature.
- Falls back to registry `HKLM\...\Winlogon\Shell` if v2 not available.
- **Configs refer to `C:\Program Files\CustomShell\WinUIShell.exe`** (NOT the old shellhost.exe).

### CI
- `.github/workflows/ci.yml` builds C++ backend + C# frontend on `windows-2022`.
- Installs MSVC via `ilammy/msvc-dev-cmd`, .NET 8 via `setup-dotnet`.
