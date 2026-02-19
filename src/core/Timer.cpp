#include "core/Timer.hpp"

Timer::Timer(double fixedDeltaSeconds)
    : m_fixedDelta(fixedDeltaSeconds), m_accumulator(0.0) {}

void Timer::tick(double frameSeconds) {
    m_accumulator += frameSeconds;
}

bool Timer::canStep() const {
    return m_accumulator >= m_fixedDelta;
}

void Timer::consumeStep() {
    m_accumulator -= m_fixedDelta;
}

double Timer::alpha() const {
    return m_accumulator / m_fixedDelta;
}

double Timer::delta() const {
    return m_fixedDelta;
}
