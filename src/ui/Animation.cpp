#include "Animation.h"

namespace shell::ui {

void Animator::Start(float from, float to, int durationMs) {
    m_from = from;
    m_to = to;
    m_durationMs = durationMs > 0 ? durationMs : 1;
    m_startTick = GetTickCount64();
}

void Animator::Stop() {
    m_durationMs = 0;
    m_from = m_to;
}

bool Animator::IsRunning() const {
    if (m_durationMs <= 0)
        return false;
    return GetTickCount64() - m_startTick < static_cast<ULONGLONG>(m_durationMs);
}

float Animator::Value() const {
    if (m_durationMs <= 0)
        return m_to;
    const ULONGLONG elapsed = GetTickCount64() - m_startTick;
    if (elapsed >= static_cast<ULONGLONG>(m_durationMs))
        return m_to;
    const float t = static_cast<float>(elapsed) / static_cast<float>(m_durationMs);
    const float inv = 1.0f - t;
    const float eased = 1.0f - inv * inv * inv; // ease-out cubic
    return m_from + (m_to - m_from) * eased;
}

} // namespace shell::ui
