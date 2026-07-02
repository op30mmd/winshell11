#pragma once
#include "Theme.h"
#include "Widget.h"
#include <memory>
#include <windows.h>

namespace shell::ui {

// Bridges a Win32 window to a widget tree. Owns the theme, the root widget,
// and a cached double buffer (same pattern as TaskbarWindow::EnsureBuffer).
//
// Usage inside a WindowProc:
//   LRESULT result = 0;
//   if (self->m_ui.HandleMessage(uMsg, wParam, lParam, result))
//       return result;
//   // ...window-specific handling, then DefWindowProc.
//
// Setup after CreateWindowExW:
//   m_ui.Attach(m_hwnd);
//   auto root = std::make_unique<StackPanel>();
//   root->AddChild(std::make_unique<Button>());
//   m_ui.SetRoot(std::move(root));
class UiHost {
public:
    ~UiHost();

    UiHost() = default;
    UiHost(const UiHost&) = delete;
    UiHost& operator=(const UiHost&) = delete;

    void Attach(HWND hwnd);
    bool IsAttached() const { return m_hwnd != nullptr; }
    void SetRoot(std::unique_ptr<Widget> root);
    Widget* GetRoot() const { return m_root.get(); }
    Theme& GetTheme() { return m_theme; }

    // Routes a window message into the widget tree. Returns true when the
    // message was fully consumed and `result` should be returned from the
    // window procedure. WM_SIZE and WM_MOUSEMOVE are processed but never
    // consumed so the host window can also react to them.
    bool HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result);

    void Invalidate();
    void PerformLayout();

    // Schedules a repaint roughly one frame (16 ms) from now. Widgets with a
    // running Animator call this from Paint to keep animating.
    void RequestAnimationFrame();

private:
    void OnPaint();
    void EnsureBuffer(int w, int h);
    void DestroyBuffer();
    void UpdateHover(POINT pt);

    static constexpr UINT_PTR kAnimTimerId = 0x5549;

    HWND m_hwnd = nullptr;
    Theme m_theme;
    std::unique_ptr<Widget> m_root;
    Widget* m_hovered = nullptr;
    Widget* m_pressed = nullptr;
    bool m_trackingMouse = false;

    // Cached double buffer.
    HDC m_bufferDC = nullptr;
    HBITMAP m_bufferBmp = nullptr;
    HBITMAP m_bufferOldBmp = nullptr;
    int m_bufferW = 0;
    int m_bufferH = 0;
};

} // namespace shell::ui
