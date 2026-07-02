#include "TaskbarWidgets.h"
#include "tray/TrayHost.h"
#include "ui/Renderer.h"
#include "ui/Theme.h"
#include "ui/UiHost.h"
#include <windowsx.h>

namespace shell::taskbar {

// ═══════════════════════════════════════════════════════════════════════
//  StartButtonWidget
// ═══════════════════════════════════════════════════════════════════════

void StartButtonWidget::Paint(ui::Renderer& renderer, const ui::Theme& theme) {
    const RECT& b = GetBounds();

    COLORREF bg = IsPressed() ? RGB(0, 100, 190) :
                  IsHovered() ? RGB(0, 135, 230) :
                  RGB(0, 120, 215);
    renderer.FillRect(b, bg);

    renderer.DrawString(L"\u25B6", b, RGB(255, 255, 255),
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX,
                        theme.fontFamily, 18, true);
}

bool StartButtonWidget::OnMouseUp(POINT pt) {
    if (PtInRect(&GetBounds(), pt) && m_onClick)
        m_onClick();
    return true;
}

// ═══════════════════════════════════════════════════════════════════════
//  TaskButtonWidget
// ═══════════════════════════════════════════════════════════════════════

void TaskButtonWidget::SetActive(bool active, float accentHeight) {
    m_active = active;
    m_accentHeight = accentHeight;
    Invalidate();
}

void TaskButtonWidget::Paint(ui::Renderer& renderer, const ui::Theme& theme) {
    const RECT& b = GetBounds();

    int r, g, b2;
    if (m_active) {
        r = IsHovered() ? 60 : 55;
        g = IsHovered() ? 60 : 55;
        b2 = IsHovered() ? 70 : 65;
    } else if (IsHovered()) {
        r = 50; g = 50; b2 = 56;
    } else {
        r = 40; g = 40; b2 = 48;
    }
    renderer.FillRect(b, RGB(r, g, b2));

    int accentH = (int)(m_accentHeight + 0.5f);
    if (accentH > 0) {
        RECT accentRc = {b.left, b.top, b.right, b.top + accentH};
        renderer.FillRect(accentRc, RGB(0, 120, 215));
    }

    RECT textRc = b;
    textRc.left += 8;
    textRc.right -= 4;
    renderer.DrawString(m_info.title, textRc, RGB(220, 220, 220),
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX,
                        theme.fontFamily, 14);
}

bool TaskButtonWidget::OnMouseUp(POINT pt) {
    if (PtInRect(&GetBounds(), pt) && m_onClick)
        m_onClick();
    return true;
}

// ═══════════════════════════════════════════════════════════════════════
//  TaskButtonsContainer
// ═══════════════════════════════════════════════════════════════════════

void TaskButtonsContainer::SetWindows(const std::vector<watcher::WindowInfo>& windows) {
    m_windows = windows;
    RebuildChildren(windows);
    Invalidate();
}

void TaskButtonsContainer::SetActiveWindow(int activeIdx) {
    if (activeIdx == m_nextActiveIdx)
        return;

    m_prevActiveIdx = m_nextActiveIdx;
    m_nextActiveIdx = activeIdx;
    m_activeIdx = activeIdx;

    if (m_prevActiveIdx >= 0 || m_nextActiveIdx >= 0)
        StartActiveAnimation(m_prevActiveIdx, m_nextActiveIdx);
}

void TaskButtonsContainer::Layout() {
    const RECT& b = GetBounds();
    int x = b.left;
    size_t count = Children().size();
    if (count == 0) return;

    int available = b.right - b.left;
    int spacing = 2;
    int btnWidth = m_buttonWidth;
    if ((int)count * btnWidth + (int)(count - 1) * spacing > available) {
        btnWidth = (available - (int)(count - 1) * spacing) / (int)count;
        if (btnWidth < 40) btnWidth = 40;
    }

    for (size_t i = 0; i < count; i++) {
        auto& child = Children()[i];
        int w = (i == count - 1) ? (b.right - x) : btnWidth;
        child->SetBounds({x, b.top, x + w, b.bottom});
        x += w + spacing;
        child->Layout();
    }
}

void TaskButtonsContainer::Paint(ui::Renderer& renderer, const ui::Theme& theme) {
    if (m_anim.IsRunning()) {
        float t = m_anim.Value();
        float accentH = 3.0f * t;

        for (size_t i = 0; i < Children().size(); i++) {
            auto* btn = static_cast<TaskButtonWidget*>(Children()[i].get());
            bool isPrev = ((int)i == m_prevActiveIdx);
            bool isNext = ((int)i == m_nextActiveIdx);
            if (isPrev)
                btn->SetActive(true, 3.0f - accentH);
            else if (isNext)
                btn->SetActive(true, accentH);
            else
                btn->SetActive(false, 0.0f);
        }

        if (GetHost())
            GetHost()->RequestAnimationFrame();
    } else {
        for (size_t i = 0; i < Children().size(); i++) {
            auto* btn = static_cast<TaskButtonWidget*>(Children()[i].get());
            btn->SetActive((int)i == m_activeIdx, 3.0f);
        }
    }

    ui::Widget::Paint(renderer, theme);
}

ui::Widget* TaskButtonsContainer::HitTest(POINT pt) {
    if (!IsVisible() || !PtInRect(&GetBounds(), pt))
        return nullptr;
    for (auto it = Children().rbegin(); it != Children().rend(); ++it) {
        if ((*it)->IsVisible() && PtInRect(&(*it)->GetBounds(), pt))
            return it->get();
    }
    return nullptr;
}

void TaskButtonsContainer::RebuildChildren(const std::vector<watcher::WindowInfo>& windows) {
    ClearChildren();

    for (size_t i = 0; i < windows.size(); i++) {
        auto btn = std::make_unique<TaskButtonWidget>();
        btn->SetWindowInfo(windows[i]);
        btn->SetOnClick([this, i]() {
            if (m_onClick)
                m_onClick((int)i);
        });
        AddChild(std::move(btn));
    }

    if (GetHost())
        GetHost()->PerformLayout();
}

void TaskButtonsContainer::StartActiveAnimation(int /*fromIdx*/, int /*toIdx*/) {
    m_anim.Start(0.0f, 1.0f, 180);
    if (GetHost())
        GetHost()->RequestAnimationFrame();
}

// ═══════════════════════════════════════════════════════════════════════
//  TrayWidget
// ═══════════════════════════════════════════════════════════════════════

void TrayWidget::Paint(ui::Renderer& renderer, const ui::Theme& /*theme*/) {
    if (m_trayHost) {
        const RECT& b = GetBounds();
        m_trayHost->Render(renderer.Handle(), b.left, 0, b.bottom - b.top);
    }
}

bool TrayWidget::OnMouseDown(POINT pt) {
    if (m_onClick)
        m_onClick(pt);
    return true;
}

// ═══════════════════════════════════════════════════════════════════════
//  ClockWidget
// ═══════════════════════════════════════════════════════════════════════

void ClockWidget::Paint(ui::Renderer& renderer, const ui::Theme& theme) {
    const RECT& b = GetBounds();

    renderer.FillRect(b, RGB(30, 30, 35));

    RECT sepRc = {b.left - 1, b.top + 4, b.left, b.bottom - 4};
    renderer.FillRect(sepRc, RGB(60, 60, 65));

    std::wstring timeStr = GetTimeString();
    size_t nl = timeStr.find(L'\n');
    std::wstring timeLine = timeStr.substr(0, nl);
    std::wstring dateLine = timeStr.substr(nl + 1);

    RECT timeRc = b;
    timeRc.top += 3;
    renderer.DrawString(timeLine, timeRc, RGB(220, 220, 220),
                        DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOPREFIX,
                        theme.fontFamily, 14);

    timeRc.top += 17;
    renderer.DrawString(dateLine, timeRc, RGB(220, 220, 220),
                        DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOPREFIX,
                        theme.fontFamily, 14);
}

bool ClockWidget::OnMouseUp(POINT pt) {
    if (PtInRect(&GetBounds(), pt) && m_onClick)
        m_onClick();
    return true;
}

std::wstring ClockWidget::GetTimeString() const {
    wchar_t buf[64];
    GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, nullptr, nullptr, buf, 64);
    wchar_t dateBuf[64];
    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, nullptr, nullptr, dateBuf, 64);
    return std::wstring(buf) + L"\n" + dateBuf;
}

// ═══════════════════════════════════════════════════════════════════════
//  TaskbarPanel
// ═══════════════════════════════════════════════════════════════════════

TaskbarPanel::TaskbarPanel() {
    // Background is drawn by the theme; no additional setup needed.
}

void TaskbarPanel::SetStartButton(std::unique_ptr<StartButtonWidget> w) {
    m_startBtn = w.get();
    AddChild(std::move(w));
}

void TaskbarPanel::SetTaskButtons(std::unique_ptr<TaskButtonsContainer> w) {
    m_taskBtns = w.get();
    AddChild(std::move(w));
}

void TaskbarPanel::SetTray(std::unique_ptr<TrayWidget> w) {
    m_tray = w.get();
    AddChild(std::move(w));
}

void TaskbarPanel::SetClock(std::unique_ptr<ClockWidget> w) {
    m_clock = w.get();
    AddChild(std::move(w));
}

void TaskbarPanel::Layout() {
    const RECT& b = GetBounds();
    int w = b.right - b.left;
    int h = b.bottom - b.top;

    int clockW = kClockWidth;
    int startW = kStartBtnWidth;
    int trayW = m_tray && m_tray->GetTrayHost() ? m_tray->GetTrayHost()->GetDesiredWidth() : 0;

    // Clock: right-aligned
    if (m_clock)
        m_clock->SetBounds({w - clockW, 0, w, h});

    // Tray: before clock
    int trayRight = w - clockW - 6;
    int trayLeft = trayRight - trayW;
    if (m_tray)
        m_tray->SetBounds({trayLeft, 0, trayRight, h});

    // Start button: left-aligned
    if (m_startBtn)
        m_startBtn->SetBounds({0, 0, startW, h});

    // Task buttons: fill remaining space
    int btnLeft = startW;
    int btnRight = trayLeft;
    if (m_taskBtns)
        m_taskBtns->SetBounds({btnLeft, 0, btnRight, h});

    Widget::Layout();
}

void TaskbarPanel::Paint(ui::Renderer& renderer, const ui::Theme& theme) {
    Widget::Paint(renderer, theme);
}

} // namespace shell::taskbar
