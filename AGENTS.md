# AGENTS.md: CustomShell — Win32 Desktop Shell Project

## 🤖 Agent Persona & Objective
**Role:** You are an AI software engineering agent assisting with a Win32-based codebase, specifically focused on custom desktop shell development.
**Objective:** Your goal is to analyze, debug, and implement Win32 UI features and shell components while **strictly adhering to the project's existing structure, abstractions, and coding conventions.**
**Mindset:** Win32 is highly stateful and flexible. Every project handles window creation, message routing, and resource management differently. Desktop shells have unique requirements regarding Z-order, workspace management, and global system hooks. Analyze the project's unique architecture before suggesting fixes.

---

## 📂 1. Project Architecture Overview

### Component Layout (single process, single thread)
All shell components run in-process on a single UI thread. The shellhost executable creates them sequentially:

```
main.cpp::WinMain
  ├── MainWindow          (message-only HWND_MESSAGE, handles WM_HOTKEY)
  ├── DesktopWindow       (full-screen WS_POPUP, HWND_BOTTOM, wallpaper + icons)
  ├── TaskbarWindow       (AppBar at bottom, HWND_TOPMOST, window list + clock + tray)
  ├── TrayHost            (notification area icon manager, no HWND)
  ├── FlyoutWindow        (popup control center, quick settings)
  ├── WindowWatcher       (WinEvent hooks for tracking open windows)
  ├── AppLauncher         (Start Menu enumeration + popup)
  └── HotkeyManager       (registered via MainWindow)
```

### Custom UI Framework (`src/ui/`)
- **`Widget`** — abstract base class with bounds, parent/child tree, `Paint()`/`Layout()`/`HitTest()`.
- **`UiHost`** — bridges a Win32 `HWND` to a widget tree; owns the `Theme`, double-buffer (`EnsureBuffer`), and message routing (`HandleMessage`).
- **`Renderer`** — GDI drawing helper (FillRect, DrawString, DrawBorder, etc.) over an HDC.
- **`Theme`** — shared visual properties (colors, font, corner radius).
- **Double-buffering:** `UiHost::OnPaint()` paints to an offscreen buffer (`CreateCompatibleDC` + `CreateCompatibleBitmap`), then `BitBlt`s to the window.

### Desktop Module (`src/desktop/`)
- **`DesktopWindow`** — Win32 window with `WS_POPUP | WS_VISIBLE | WS_EX_TOOLWINDOW`, full-screen at `HWND_BOTTOM`. Routes messages through `UiHost`.
- **`DesktopPanel`** — root widget; renders wallpaper (GDI+ `Image`) or solid color `RGB(30,30,36)`, then paints icon children.
- **`DesktopIconWidget`** — renders a 48×48 icon via `DrawIconEx` + text label via `DrawTextW`.
- **`IconManager`** — enumerates desktop items via `SHGetDesktopFolder` → `EnumObjects`, computes grid positions.

### Taskbar Module (`src/taskbar/`)
- **`TaskbarWindow`** — AppBar at bottom via `SHAppBarMessage(ABM_NEW/ABM_SETPOS)`, `WS_EX_TOPMOST | WS_EX_TOOLWINDOW`.
- **`TaskbarPanel`** — horizontal strip with Start button, task buttons (window list), tray, clock.

---

## 📐 2. General Rules of Engagement
1.  **Paint system is fragile.** Never assume `InvalidateRect` triggers `WM_PAINT` — the thread runs a timer (`kAnimTimerId` + Taskbar 1-second timer) that can prevent `GetMessage` from synthesizing `WM_PAINT`. Always use `RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE)` to force a synchronous paint.
2.  **Stay within the existing architecture:** Do not introduce alternative frameworks or new dependencies unless explicitly instructed. Fix bugs using the tools currently available in the project.
3.  **Match the Coding Style:** Adapt your output to match the surrounding code exactly (C++20, namespaces `shell::*`, `std::wstring` for Win32 strings, `COLORREF` not `Gdiplus::Color` for GDI).
4.  **Use Project Error Handling:** Use the project's existing macros (`LOG_INFO`, `LOG_WARNING`, `LOG_ERROR`) which now support printf-style format strings and `OutputDebugStringA`.
5.  **Build system:** CMake + MSVC with `/W4 /WX /analyze`. Build from repo root: `cmake --build build --config Release`.

---

## 🖥️ 3. CustomShell Design Rules

### A. Paint & Invalidation 🔑
- **NEVER use `InvalidateRect` alone** — it marks the update region but `GetMessage` may never synthesize `WM_PAINT` because the thread's message queue never fully drains (periodic timers from TaskbarWindow keep it busy).
- **Always use `RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE)`** to force an immediate synchronous paint. This is implemented in `UiHost::Invalidate()`.
- The double-buffer is created once at the window's client size and reused. GDI+ wallpaper is painted via `Graphics::DrawImage` on top of the GDI background fill; GDI icon/text rendering follows.

### B. Desktop Window
- Full-screen `WS_POPUP | WS_VISIBLE | WS_EX_TOOLWINDOW` at `HWND_BOTTOM`.
- Wallpaper is loaded from the registry `HKCU\Control Panel\Desktop\Wallpaper` via `Config::Load()`.
- Icons are enumerated via `SHGetDesktopFolder` → `EnumObjects` (includes hidden items), positioned in a grid (100×96 spacing, 12px margin).
- `DesktopIconData` struct owns raw `HICON` and `LPITEMIDLIST` pointers; cleanup happens in `DesktopPanel::ClearIcons()`.
- If you modify `SetIcons()`, do NOT null-out pointers after `std::move` — `DesktopIconData` has no destructor, and the moved-from source is empty.

### C. Taskbar Window
- Registered as an AppBar with `SHAppBarMessage(ABM_NEW)` and `ABM_SETPOS`.
- Forces `HWND_TOPMOST` in `WM_WINDOWPOSCHANGING`.
- 1-second timer for clock refresh and active-window tracking.
- Tray area delegates to `TrayHost` for icon rendering and click handling.

### D. Workspace and AppBar Management
*   **Do not obscure maximized windows:** Shell panels (like taskbars) must register as AppBars using `SHAppBarMessage` (`ABM_NEW`, `ABM_SETPOS`). 
*   **Work Area Validation:** If modifying the desktop work area directly, ensure the code uses `SystemParametersInfo(SPI_SETWORKAREA, ...)` correctly and broadcasts `WM_SETTINGCHANGE`.
*   **Display Changes:** The shell must gracefully handle `WM_DISPLAYCHANGE` and `WM_DPICHANGED` to reposition AppBars.

### E. Global Window Tracking (Taskbar behavior)
*   **Window Hooks:** This project uses `SetWinEventHook` (not `RegisterShellHookWindow`) for tracking window creation/activation. See `WindowWatcher`.
*   **Window Filtering:** Properly filter tracked windows (Check `IsWindowVisible`, `WS_EX_TOOLWINDOW`, `WS_EX_APPWINDOW`, and `GetWindow(hwnd, GW_OWNER)`).

### F. Z-Order, Focus, and Activation
*   **Non-Intrusive Overlays:** Launchers and search menus should use `WS_EX_NOACTIVATE` to avoid stealing keyboard focus when clicked, unless text input is explicitly required.
*   **Layering Rules:** Desktop sits at `HWND_BOTTOM`. Taskbar sits at `HWND_TOPMOST` but yields to full-screen exclusive apps via `ABN_FULLSCREENAPP`.

---

## 🔍 4. Debugging Approach (Headless CLI First)
When tasked with a bug, **you must attempt to debug autonomously in headless mode via the CLI first.** Do not ask the user for manual help unless CLI automation fails.

1.  **Hypothesize & Plan:** Analyze the symptom (blank desktop, missing icons, hang, crash, resource leak).
2.  **Headless Diagnostic Execution:** Use terminal commands (PowerShell, `cdb.exe`, procdump, etc.) to gather memory dumps, handle counts, and call stacks.
3.  **Trace Code Logic:** Follow the state changes in the code (`WS_VISIBLE`, Z-order, `WndProc` logic).
4.  **Inject logging** via the project's variadic `LOG_INFO(...)` macros (they write to `OutputDebugStringA` and stdout). Rebuild via `cmake --build build --config Release`.
5.  **Fallback to User (Stop & Ask):** *If and only if* the headless CLI tools fail to reveal the issue, or if the issue requires visual inspection that a terminal cannot provide (e.g., specific UI rendering artifacts), **stop executing commands and ask the user for help** using GUI tools.

---

## 🛠️ 5. Using Windows Debugging Tools Correctly

### Phase 1: Agent-Driven Headless CLI Debugging (Primary)
Use the terminal environment to execute these tools. **Do this autonomously before asking the user for help.**

*   **CDB (Command-line Debugger):** Path: `"C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe"`. Always use `bm` (pattern breakpoint) instead of `bp` because C++ name mangling makes fully qualified names unpredictable:
    ```
    bm shellhost!*DesktopPanel*Paint
    bm shellhost!*UiHost*Invalidate
    ```
    *   *Quick trace (run for N seconds then dump):*
        ```
        cdb -o -g -c "bm shellhost!*DrawIcon*; g 10s; bl; ~*kv; q" shellhost.exe
        ```
    *   *Check if Paint is called with correct children count (critical for desktop icons):*
        ```
        cdb -o -g -c "bm shellhost!*DesktopPanel*Paint; g 10s; bl; q" shellhost.exe
        ```
        Read the `OutputDebugString` output in CDB's stdout — look for log lines like `DesktopPanel::Paint called, children=76 icons=76`.
    *   *Analyze Dumps:* Run `cdb -z crash.dmp -c "!analyze -v; q"` to automatically parse a crash dump without opening a GUI.
    *   *Symbol loading is deferred by default; use `.symopt+ 0x100` and `ld *` to force full symbol load.*
*   **PowerShell for Resource Leaks:** Query handles and threads without Task Manager.
    *   Run `Get-Process shellhost | Select-Object Handles, Threads` to check for resource exhaustion.
*   **Sysinternals CLI (if available):**
    *   Use `procdump -ma -h shellhost.exe` to generate a dump if a window hangs (Not Responding).
    *   Use `handle.exe -p shellhost.exe` to see if the process is hoarding specific objects.
*   **Log Injection:** The LOG macros now support printf-style format strings (`LOG_INFO("icons=%zu", count)`) and output to both stdout and `OutputDebugStringA`. Rebuild with `cmake --build build --config Release`. Read output via CDB (stdout is captured) or DebugView (`dbgview.exe`).

### Known CDB Pitfalls for this Project
| Issue | Solution |
|-------|----------|
| C++ name mangling breaks `bp` | Use `bm shellhost!*FuncName*` instead |
| `Shell host initialized.` appears but no Paint | Check `InvalidateRect` — replace with `RedrawWindow` |
| No PDB symbols | Build with RelWithDebInfo config: `cmake --build build --config RelWithDebInfo` |
| Output truncated | Don't use `-z` (dump mode) for live debugging; use `-o -g` |

### Phase 2: User-Assisted GUI Debugging (Fallback)
**STOP AND ASK THE USER:** If your CLI checks (CDB, PowerShell) fail, or if the bug is purely visual/Z-order related and requires inspection, halt your actions. Ask the user to run the following GUI tools and report the output:

*   **Spy++ (`spyxx.exe`):** 
    *   Ask the user to inspect the Window Tree. Request the exact **Styles**, **Extended Styles**, and Z-order of the problematic window.
    *   Ask the user to log messages for the specific window (instruct them to filter out high-volume messages like `WM_MOUSEMOVE` and `WM_TIMER`).
*   **Application Verifier (AppVerif):**
    *   If you suspect elusive memory corruption or invalid handle usage (`STATUS_INVALID_HANDLE`), ask the user to add the executable to Application Verifier (enable Basics: Handles, Heaps, Locks) and run it under a GUI debugger.
*   **Visual WinDbg / Visual Studio:**
    *   For deep interactive stepping that `cdb` scripts cannot easily handle.

---

## ⚠️ 6. Common Pitfalls to Avoid in Proposals
*   **Stealing Focus Unintentionally:** Popping up shell notifications without `SW_SHOWNOACTIVATE` / `WS_EX_NOACTIVATE`.
*   **Improper Fullscreen Handling:** Failing to yield `HWND_TOPMOST` status when another application (like a game) goes fullscreen.
*   **Leaking Icons:** Calling `ExtractIcon`/`SHGetFileInfo(SHGFI_ICON)` without calling `DestroyIcon()`.
*   **Failing to call `DefWindowProc`:** (or the project's equivalent) for unhandled messages.
*   **Ignoring high-DPI (Per-Monitor v2) scaling:** Desktop shells span multiple monitors with different scale factors. Use `GetSystemMetricsForDpi` instead of standard `GetSystemMetrics`.
*   **Hardcoding string types:** Respect the project's usage of `std::wstring` for Win32 APIs and use the correct suffix (`-W` APIs, since `UNICODE` is defined).
*   **Using `InvalidateRect` instead of `RedrawWindow`:** In this project, `InvalidateRect` with `FALSE` erase may never trigger `WM_PAINT` due to the message queue never fully draining (timers from TaskbarWindow). Always use `RedrawWindow(..., RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE)`.
*   **Assuming `Config::Load()` reads a config file:** It returns struct defaults — only `wallpaperPath` is populated (from registry). There is no JSON config parser yet. The sample `shell.config.json.sample` is not read at runtime.
*   **Modularizing `DesktopIconData` ownership:** The struct holds raw `HICON` and `LPITEMIDLIST` pointers with no destructor. Ownership is managed by `DesktopPanel::m_icons` (which frees in `ClearIcons`). Widget children borrow the pointers. Never free them from outside `ClearIcons`.