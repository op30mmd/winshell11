#include "UiHost.h"
#include "Renderer.h"
#include <windowsx.h>

namespace shell::ui {

UiHost::~UiHost() {
    DestroyBuffer();
}

void UiHost::Attach(HWND hwnd) {
    m_hwnd = hwnd;
    if (m_root)
        PerformLayout();
}

void UiHost::SetRoot(std::unique_ptr<Widget> root) {
    m_hovered = nullptr;
    m_pressed = nullptr;
    m_root = std::move(root);
    if (m_root)
        m_root->SetHostRecursive(this);
    if (m_hwnd)
        PerformLayout();
}

void UiHost::Invalidate() {
    if (m_hwnd) {
        // Use RedrawWindow to reliably generate WM_PAINT (InvalidateRect can
        // fail to trigger a paint when the message queue is never fully idle,
        // e.g. when a periodic timer runs on the same thread).
        RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    }
}

void UiHost::PerformLayout() {
    if (!m_hwnd || !m_root)
        return;
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    m_root->SetBounds(rc);
    m_root->Layout();
    Invalidate();
}

void UiHost::RequestAnimationFrame() {
    if (m_hwnd)
        SetTimer(m_hwnd, kAnimTimerId, 16, nullptr);
}

bool UiHost::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result) {
    switch (msg) {
    case WM_PAINT:
        OnPaint();
        result = 0;
        return true;

    case WM_ERASEBKGND:
        result = 1;
        return true;

    case WM_SIZE:
        PerformLayout();
        return false; // Not consumed: the window may want WM_SIZE too.

    case WM_MOUSEMOVE: {
        if (!m_trackingMouse) {
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, m_hwnd, 0};
            TrackMouseEvent(&tme);
            m_trackingMouse = true;
        }
        const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if (m_pressed)
            m_pressed->OnMouseMove(pt);
        else {
            UpdateHover(pt);
            if (m_hovered)
                m_hovered->OnMouseMove(pt);
        }
        return false; // Not consumed: the window may want WM_MOUSEMOVE too.
    }

    case WM_MOUSELEAVE:
        m_trackingMouse = false;
        if (m_hovered) {
            m_hovered->SetHoverState(false);
            m_hovered = nullptr;
        }
        return false;

    case WM_LBUTTONDOWN: {
        const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        Widget* hit = m_root ? m_root->HitTest(pt) : nullptr;
        if (hit) {
            m_pressed = hit;
            hit->SetPressedState(true);
            SetCapture(m_hwnd);
            hit->OnMouseDown(pt);
            result = 0;
            return true;
        }
        return false;
    }

    case WM_LBUTTONUP: {
        if (m_pressed) {
            const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ReleaseCapture();
            Widget* pressed = m_pressed;
            m_pressed = nullptr;
            pressed->OnMouseUp(pt);
            pressed->SetPressedState(false);
            UpdateHover(pt);
            result = 0;
            return true;
        }
        return false;
    }

    case WM_TIMER:
        if (wParam == kAnimTimerId) {
            KillTimer(m_hwnd, kAnimTimerId);
            Invalidate();
            result = 0;
            return true;
        }
        return false;

    case WM_DESTROY:
        DestroyBuffer();
        return false;

    default:
        return false;
    }
}

void UiHost::UpdateHover(POINT pt) {
    Widget* hit = m_root ? m_root->HitTest(pt) : nullptr;
    if (hit != m_hovered) {
        if (m_hovered)
            m_hovered->SetHoverState(false);
        m_hovered = hit;
        if (m_hovered)
            m_hovered->SetHoverState(true);
    }
}

void UiHost::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    const int w = rc.right - rc.left;
    const int h = rc.bottom - rc.top;

    if (w > 0 && h > 0) {
        EnsureBuffer(w, h);
        if (m_bufferDC) {
            Renderer renderer(m_bufferDC);
            renderer.FillRect(rc, m_theme.background);
            if (m_root && m_root->IsVisible())
                m_root->Paint(renderer, m_theme);
            BitBlt(hdc, 0, 0, w, h, m_bufferDC, 0, 0, SRCCOPY);
        }
    }

    EndPaint(m_hwnd, &ps);
}

void UiHost::EnsureBuffer(int w, int h) {
    if (m_bufferDC && m_bufferW >= w && m_bufferH >= h)
        return;
    DestroyBuffer();
    HDC screenDC = GetDC(nullptr);
    m_bufferDC = CreateCompatibleDC(screenDC);
    m_bufferBmp = CreateCompatibleBitmap(screenDC, w, h);
    m_bufferOldBmp = (HBITMAP)SelectObject(m_bufferDC, m_bufferBmp);
    m_bufferW = w;
    m_bufferH = h;
    ReleaseDC(nullptr, screenDC);
}

void UiHost::DestroyBuffer() {
    if (m_bufferDC) {
        SelectObject(m_bufferDC, m_bufferOldBmp);
        DeleteObject(m_bufferBmp);
        DeleteDC(m_bufferDC);
        m_bufferDC = nullptr;
        m_bufferBmp = nullptr;
        m_bufferOldBmp = nullptr;
        m_bufferW = 0;
        m_bufferH = 0;
    }
}

} // namespace shell::ui
