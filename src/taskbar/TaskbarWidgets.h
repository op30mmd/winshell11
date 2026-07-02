#pragma once
#include "ui/Widget.h"
#include "ui/Animation.h"
#include "windowwatcher/WindowWatcher.h"
#include <functional>
#include <string>
#include <vector>

namespace shell::tray {
class TrayHost;
}

namespace shell::taskbar {

// ── StartButtonWidget ──────────────────────────────────────────────────
class StartButtonWidget : public ui::Widget {
public:
    using ClickHandler = std::function<void()>;
    void SetOnClick(ClickHandler handler) { m_onClick = std::move(handler); }
    void Paint(ui::Renderer& renderer, const ui::Theme& theme) override;
    bool OnMouseDown(POINT) override { return true; }
    bool OnMouseUp(POINT pt) override;
private:
    ClickHandler m_onClick;
};

// ── TaskButtonWidget ───────────────────────────────────────────────────
class TaskButtonWidget : public ui::Widget {
public:
    using ClickHandler = std::function<void()>;

    void SetWindowInfo(const watcher::WindowInfo& info) { m_info = info; }
    const watcher::WindowInfo& GetWindowInfo() const { return m_info; }
    void SetActive(bool active, float accentHeight = 3.0f);
    bool IsActive() const { return m_active; }
    float GetAccentHeight() const { return m_accentHeight; }
    void SetOnClick(ClickHandler handler) { m_onClick = std::move(handler); }

    void Paint(ui::Renderer& renderer, const ui::Theme& theme) override;
    bool OnMouseDown(POINT) override { return true; }
    bool OnMouseUp(POINT pt) override;

private:
    watcher::WindowInfo m_info;
    bool m_active = false;
    float m_accentHeight = 0.0f;
    ClickHandler m_onClick;
};

// ── TaskButtonsContainer ───────────────────────────────────────────────
class TaskButtonsContainer : public ui::Widget {
public:
    using WindowClickHandler = std::function<void(int)>;

    void SetWindows(const std::vector<watcher::WindowInfo>& windows);
    void SetActiveWindow(int activeIdx);
    int GetActiveWindow() const { return m_activeIdx; }
    void SetOnWindowClick(WindowClickHandler handler) { m_onClick = std::move(handler); }

    void Layout() override;
    void Paint(ui::Renderer& renderer, const ui::Theme& theme) override;

    // HitTest override to prevent returning this container instead of a button child
    ui::Widget* HitTest(POINT pt) override;

private:
    void RebuildChildren(const std::vector<watcher::WindowInfo>& windows);
    void StartActiveAnimation(int fromIdx, int toIdx);

    std::vector<watcher::WindowInfo> m_windows;
    int m_activeIdx = -1;
    int m_prevActiveIdx = -1;
    int m_nextActiveIdx = -1;
    int m_buttonWidth = 160;
    ui::Animator m_anim;
    WindowClickHandler m_onClick;
};

// ── TrayWidget ─────────────────────────────────────────────────────────
class TrayWidget : public ui::Widget {
public:
    void SetTrayHost(tray::TrayHost* host) { m_trayHost = host; }
    tray::TrayHost* GetTrayHost() const { return m_trayHost; }
    void Paint(ui::Renderer& renderer, const ui::Theme& theme) override;
    void SetOnClick(std::function<void(POINT)> handler) { m_onClick = std::move(handler); }
    bool OnMouseDown(POINT pt) override;
    bool OnMouseUp(POINT) override { return true; }
private:
    tray::TrayHost* m_trayHost = nullptr;
    std::function<void(POINT)> m_onClick;
};

// ── ClockWidget ────────────────────────────────────────────────────────
class ClockWidget : public ui::Widget {
public:
    using ClickHandler = std::function<void()>;
    void SetOnClick(ClickHandler handler) { m_onClick = std::move(handler); }
    void Paint(ui::Renderer& renderer, const ui::Theme& theme) override;
    bool OnMouseDown(POINT) override { return true; }
    bool OnMouseUp(POINT) override;
    std::wstring GetTimeString() const;
private:
    ClickHandler m_onClick;
};

// ── TaskbarPanel (root widget for the taskbar window) ──────────────────
class TaskbarPanel : public ui::Widget {
public:
    TaskbarPanel();

    // Setters transfer ownership, return raw pointers for wiring callbacks.
    void SetStartButton(std::unique_ptr<StartButtonWidget> w);
    void SetTaskButtons(std::unique_ptr<TaskButtonsContainer> w);
    void SetTray(std::unique_ptr<TrayWidget> w);
    void SetClock(std::unique_ptr<ClockWidget> w);

    void Layout() override;
    void Paint(ui::Renderer& renderer, const ui::Theme& theme) override;
    bool OnMouseDown(POINT) override { return false; } // never consume

    StartButtonWidget* GetStartButton() const { return m_startBtn; }
    TaskButtonsContainer* GetTaskButtons() const { return m_taskBtns; }
    TrayWidget* GetTray() const { return m_tray; }
    ClockWidget* GetClock() const { return m_clock; }

    static constexpr int kStartBtnWidth = 48;
    static constexpr int kClockWidth = 100;

private:
    StartButtonWidget* m_startBtn = nullptr;
    TaskButtonsContainer* m_taskBtns = nullptr;
    TrayWidget* m_tray = nullptr;
    ClockWidget* m_clock = nullptr;
};

} // namespace shell::taskbar
