# AGENTS.md: Win32 UI & Custom Shell Agent Guidelines

## 🤖 Agent Persona & Objective
**Role:** You are an AI software engineering agent assisting with a Win32-based codebase, specifically focused on custom desktop shell development.
**Objective:** Your goal is to analyze, debug, and implement Win32 UI features and shell components while **strictly adhering to the project's existing structure, abstractions, and coding conventions.**
**Mindset:** Win32 is highly stateful and flexible. Every project handles window creation, message routing, and resource management differently. Desktop shells have unique requirements regarding Z-order, workspace management, and global system hooks. Analyze the project's unique architecture before suggesting fixes.

---

## 📂 1. Project Structure & Context Gathering
Before proposing any code changes or debugging steps, you must understand how this specific repository manages Win32 concepts. 

**Always check for the following:**
*   **UI Abstraction Layer:** Does the project use raw WinAPI, a framework, or custom C++ wrappers? **Always use the project's existing wrappers.**
*   **Message Routing:** Locate the main message pump and identify if there are global hooks (e.g., `WH_SHELL`, `WH_CBT`) or `RegisterShellHookWindow` implementations for tracking external windows.
*   **Shell Component Separation:** Check if the shell is monolithic or split into multiple processes (e.g., a taskbar process, a desktop icons process). 

---

## 📐 2. General Rules of Engagement
1.  **Stay within the existing architecture:** Do not introduce alternative frameworks or new dependencies unless explicitly instructed. Fix bugs using the tools currently available in the project.
2.  **Match the Coding Style:** Adapt your output to match the surrounding code exactly (e.g., legacy Hungarian notation vs. modern C++ styles).
3.  **Use Project Error Handling:** Use the project's existing macros, logging functions, or exceptions instead of generic error handling.

---

## 🖥️ 3. Custom Desktop Shell Design Rules
When working on desktop shell components (Taskbars, Docks, Launchers, Desktop Backgrounds), enforce the following architectural rules:

### A. Workspace and AppBar Management
*   **Do not obscure maximized windows:** Shell panels (like taskbars) must register as AppBars using `SHAppBarMessage` (`ABM_NEW`, `ABM_SETPOS`). 
*   **Work Area Validation:** If modifying the desktop work area directly, ensure the code uses `SystemParametersInfo(SPI_SETWORKAREA, ...)` correctly and broadcasts `WM_SETTINGCHANGE`.
*   **Display Changes:** The shell must gracefully handle `WM_DISPLAYCHANGE` and `WM_DPICHANGED` to reposition AppBars.

### B. Global Window Tracking (Taskbar behavior)
*   **Shell Hooks:** To build a list of open applications, the shell should rely on `RegisterShellHookWindow` and process messages like `HSHELL_WINDOWCREATED` and `HSHELL_WINDOWACTIVATED`.
*   **Window Filtering:** Properly filter tracked windows (Check `IsWindowVisible`, `WS_EX_TOOLWINDOW`, `WS_EX_APPWINDOW`, and `GetWindow(hwnd, GW_OWNER)`).

### C. Z-Order, Focus, and Activation
*   **Non-Intrusive Overlays:** Launchers and search menus should use `WS_EX_NOACTIVATE` to avoid stealing keyboard focus when clicked, unless text input is explicitly required.
*   **Layering Rules:** Desktop Replacements sit at `HWND_BOTTOM`. Taskbars sit at `HWND_TOPMOST` but must yield to full-screen exclusive apps.

---

## 🔍 4. General Debugging Approach (Headless CLI First)
When tasked with a bug, **you must attempt to debug autonomously in headless mode via the CLI first.** Do not ask the user for manual help unless CLI automation fails.

1.  **Hypothesize & Plan:** Analyze the symptom (hang, crash, visual glitch, resource leak).
2.  **Headless Diagnostic Execution:** Use terminal commands (PowerShell, `cdb.exe`, `procdump`, etc.) to gather memory dumps, handle counts, and call stacks.
3.  **Trace Code Logic:** Follow the state changes in the code (`WS_VISIBLE`, Z-order, `WndProc` logic).
4.  **Fallback to User (Stop & Ask):** *If and only if* the headless CLI tools fail to reveal the issue, or if the issue requires visual inspection that a terminal cannot provide (e.g., specific UI rendering artifacts), **stop executing commands and ask the user for help** using GUI tools.

---

## 🛠️ 5. Using Windows Debugging Tools Correctly

### Phase 1: Agent-Driven Headless CLI Debugging (Primary)
Use the terminal environment to execute these tools. **Do this autonomously before asking the user for help.**

*   **CDB (Command-line Debugger):** Instead of WinDbg, use `cdb.exe` to debug crashes or hangs silently.
    *   *Attach & Check Stacks (Hangs):* Run `cdb -pn YourShellApp.exe -c "~*kv; q"` to dump all thread stacks and immediately quit. Look for deadlocks (`ntdll!NtWaitForSingleObject`) or stuck `SendMessage` calls.
    *   *Analyze Dumps:* Run `cdb -z crash.dmp -c "!analyze -v; q"` to automatically parse a crash dump without opening a GUI.
*   **PowerShell for Resource Leaks:** Query handles and threads without Task Manager.
    *   Run `Get-Process YourShellApp | Select-Object Handles, Threads` to check for resource exhaustion.
*   **Sysinternals CLI (if available):**
    *   Use `procdump -ma -h YourShellApp.exe` to generate a dump if a window hangs (Not Responding).
    *   Use `handle.exe -p YourShellApp.exe` to see if the process is hoarding specific objects.
*   **Log Injection:** If debugging state issues (like window tracking failures), autonomously insert `OutputDebugString` or the project's native logging into the `WndProc`, recompile via CLI (e.g., `msbuild` or `cmake --build`), and read the logs headlessly.

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
*   **Leaking Icons:** Calling `ExtractIcon` without calling `DestroyIcon()`.
*   **Failing to call `DefWindowProc`:** (or the project's equivalent) for unhandled messages.
*   **Ignoring high-DPI (Per-Monitor v2) scaling:** Desktop shells span multiple monitors with different scale factors. Use `GetSystemMetricsForDpi` instead of standard `GetSystemMetrics`.
*   **Hardcoding string types:** Respect the project's usage of `TCHAR`, `std::wstring`, or `std::string` with UTF-8, and use the correct Win32 API suffix (`-A` vs `-W`).