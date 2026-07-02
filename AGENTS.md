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
*   **Shell Component Separation:** Check if the shell is monolithic or split into multiple processes (e.g., a process for the taskbar, a process for desktop icons, a process for system hooks). 

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
*   **Work Area Validation:** If the project modifies the desktop work area directly instead of using AppBars, ensure it uses `SystemParametersInfo(SPI_SETWORKAREA, ...)` correctly and broadcasts `WM_SETTINGCHANGE` to all top-level windows.
*   **Display Changes:** The shell must gracefully handle `WM_DISPLAYCHANGE` and `WM_DPICHANGED` to reposition AppBars when monitors are plugged in, unplugged, or have their resolution changed.

### B. Global Window Tracking (Taskbar behavior)
*   **Shell Hooks:** To build a list of open applications, the shell should rely on `RegisterShellHookWindow` and process `Window Message` IDs like `HSHELL_WINDOWCREATED`, `HSHELL_WINDOWDESTROYED`, and `HSHELL_WINDOWACTIVATED`.
*   **Window Filtering:** Not every window belongs on a taskbar. Ensure the logic correctly filters windows by checking:
    *   Is it visible? (`IsWindowVisible`)
    *   Does it have `WS_EX_TOOLWINDOW`? (Usually hidden from taskbar).
    *   Does it have `WS_EX_APPWINDOW`? (Usually forced onto taskbar).
    *   Is it an owned window? (`GetWindow(hwnd, GW_OWNER)`)

### C. Z-Order, Focus, and Activation
*   **Non-Intrusive Overlays:** Launchers, search menus, and volume sliders should often use `WS_EX_NOACTIVATE` so they do not steal keyboard focus from the active application when clicked, unless text input is explicitly required.
*   **Layering Rules:** 
    *   *Desktop Replacements:* Must sit at the absolute bottom. Usually achieved via `SetWindowPos(hwnd, HWND_BOTTOM, ...)` or by becoming a child of `Progman`.
    *   *Taskbars/Docks:* Usually sit at `HWND_TOPMOST` but must handle scenarios where full-screen apps (like games) need to cover them.
*   **Foreground Management:** A shell often needs to activate other windows. Because of Foreground Lock Timeout, ensure the project uses proper techniques (like `AllowSetForegroundWindow` or thread input attachment if necessary) to bring user-selected apps to the front.

---

## 🧠 4. Core Win32 Debugging Principles
*   **Thread Affinity:** A window (`HWND`) belongs to the thread that created it. Cross-thread UI updates must be marshaled (typically via `PostMessage` or a custom project-specific dispatcher).
*   **Message Pump Starvation:** If the shell UI is hanging, look for blocking operations (I/O, heavy compute, long locks) happening on the UI thread.
*   **Resource Leaks (GDI/USER):** Shells run indefinitely. A small GDI/USER object leak will eventually crash the OS UI. Ensure objects (`HDC`, `HBITMAP`, `HICON`) are released. 

---

## 🔍 5. General Debugging Approach
When asked to debug an issue, follow this generalized workflow:

1.  **Analyze the Symptom:** 
    *   *Hang/Freeze:* Locate the UI thread. Look for thread synchronization issues or blocking calls.
    *   *Z-Order/Focus issues:* Trace where `SetFocus`, `SetForegroundWindow`, or `SetWindowPos` are called. 
    *   *Window Tracking Failures:* Check the `WndProc` handling the Shell Hook messages. Is the message ID registered properly?
2.  **Locate the Entry Point:** Find the specific `WndProc` or message handler for the affected control/window/hook.
3.  **Trace State Changes:** Check the sequence of window styles (`WS_VISIBLE`, `WS_POPUP`), Z-order, and enablement status.
4.  **Request Tooling Data if Needed:** If the bug is not visible in the code structure, ask the user to provide context from standard tools (see Section 6).

---

## 🛠️ 6. Using Windows Debugging Tools Correctly
As an AI agent, you cannot run native GUI tools yourself. When source code analysis isn't enough, **instruct the user** to use the following standard Windows debugging tools and report the output back to you.

### A. Spy++ (`spyxx.exe`)
Use this for UI layout, Z-order, and window message stream analysis.
*   **To debug Z-order/Visibility issues:** Ask the user to find their window in the Spy++ Window Tree and report the **Styles** and **Extended Styles** (e.g., is `WS_VISIBLE` missing? Is `WS_EX_LAYERED` applied but `SetLayeredWindowAttributes` not called?).
*   **To debug Hangs/Message Loops:** Ask the user to log messages for the specific window. *Rule: Tell them to filter out high-volume messages like `WM_MOUSEMOVE`, `WM_NCHITTEST`, and `WM_TIMER` to avoid noise.*
*   **To debug Resizing/Painting:** Ask the user to capture the exact sequence of `WM_WINDOWPOSCHANGING`, `WM_NCCALCSIZE`, and `WM_PAINT` messages.

### B. WinDbg / CDB
Use this for hard crashes (AVs), hangs, deadlocks, and severe memory leaks. Instruct the user to attach WinDbg or open a crash dump (`.dmp`) and run specific commands:
*   **For UI Hangs ("Not Responding"):** Ask the user to break into the debugger and run `~*kv` (list all thread stacks). Look for threads waiting on `ntdll!NtWaitForSingleObject` while holding a lock that the UI thread needs, or the UI thread stuck in `user32!SendMessageW`.
*   **For Crashes:** Ask for the output of `!analyze -v`.
*   **For Handle Leaks:** Ask the user to enable handle tracing (`!htrace -enable`), reproduce the leak, and report the output of `!htrace -diff`.

### C. Process Explorer / Task Manager
Use this for diagnosing resource leaks that degrade system performance over time.
*   **GDI & USER Object Leaks:** Instruct the user to add the "GDI Objects" and "USER Objects" columns in Task Manager or Process Explorer. If the count climbs steadily towards 10,000 during standard operation, you know there is a resource leak (e.g., missing `DeleteObject` or `DestroyWindow`).

### D. Application Verifier (AppVerif)
Use this for elusive memory corruption, invalid handle usage, or lock usage errors.
*   If the user reports random crashes or `STATUS_INVALID_HANDLE` exceptions, instruct them to add their executable to Application Verifier, enable the **Basics** (Handles, Heaps, Locks) checks, run the app under a debugger, and report where the debugger breaks.

---

## ⚠️ 7. Common Pitfalls to Avoid in Proposals
When generating code or fixing bugs, ensure you do not introduce these common Win32 and Shell errors:
*   **Stealing Focus Unintentionally:** Popping up shell notifications or background windows without `SW_SHOWNOACTIVATE` / `WS_EX_NOACTIVATE`, pulling the user out of their current application or game.
*   **Improper Fullscreen Handling:** Failing to yield `HWND_TOPMOST` status when another application goes fullscreen (e.g., a YouTube video or a game). The shell must detect fullscreen apps and drop its Z-order accordingly.
*   **Leaking Icons:** Calling `ExtractIcon` or requesting `HICON` handles for taskbar items and failing to call `DestroyIcon()`.
*   **Failing to call `DefWindowProc`:** (or the project's equivalent base handler) for unhandled messages.
*   **Ignoring high-DPI (Per-Monitor v2) scaling:** Desktop shells span multiple monitors with potentially different scale factors. Always check if the project uses `GetSystemMetricsForDpi` or standard `GetSystemMetrics`.
*   **Hardcoding string types:** Respect the project's usage of `TCHAR`, `std::wstring`, or `std::string` with UTF-8, and use the correct Win32 API suffix (`-A` vs `-W`).