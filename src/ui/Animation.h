#pragma once
#include <windows.h>

namespace shell::ui {

// Tick-based value animation with ease-out cubic interpolation.
//
// Typical usage inside a widget:
//   m_anim.Start(0.0f, 1.0f, 180);
//   ...
//   // In Paint():
//   float t = m_anim.Value();
//   if (m_anim.IsRunning() && GetHost())
//       GetHost()->RequestAnimationFrame();
class Animator {
public:
    void Start(float from, float to, int durationMs);
    void Stop();
    bool IsRunning() const;
    float Value() const;
    float Target() const { return m_to; }

private:
    float m_from = 0.0f;
    float m_to = 0.0f;
    ULONGLONG m_startTick = 0;
    int m_durationMs = 0;
};

} // namespace shell::ui
