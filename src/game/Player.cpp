#include "game/Player.hpp"

#include "game/Map.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kTau = 6.28318530718F;
}

void Player::setPosition(float x, float y) {
    m_x = x;
    m_y = y;
}

void Player::update(const InputState& input, const Map& map, float dtSeconds) {
    float dx = 0.0F;
    float dy = 0.0F;

    if (input.up) {
        dy -= 1.0F;
    }
    if (input.down) {
        dy += 1.0F;
    }
    if (input.left) {
        dx -= 1.0F;
    }
    if (input.right) {
        dx += 1.0F;
    }

    const float length = std::sqrt(dx * dx + dy * dy);
    if (length > 0.0F) {
        dx /= length;
        dy /= length;
    }

    const float candidateX = m_x + dx * m_speed * dtSeconds;
    const float candidateY = m_y + dy * m_speed * dtSeconds;

    if (!map.isBlocked(static_cast<int>(candidateX), static_cast<int>(m_y))) {
        m_x = candidateX;
    }
    if (!map.isBlocked(static_cast<int>(m_x), static_cast<int>(candidateY))) {
        m_y = candidateY;
    }

    const float targetBlend = length > 0.0F ? 1.0F : 0.0F;
    const float blendRate = 8.0F;
    m_moveBlend += (targetBlend - m_moveBlend) * std::min(1.0F, blendRate * dtSeconds);

    if (length > 0.0F) {
        const float walkCyclesPerSecond = 2.4F;
        m_walkPhase += kTau * walkCyclesPerSecond * dtSeconds;
        if (m_walkPhase >= kTau) {
            m_walkPhase = std::fmod(m_walkPhase, kTau);
        }
    }
}

float Player::x() const { return m_x; }
float Player::y() const { return m_y; }
float Player::walkPhase() const { return m_walkPhase; }
float Player::moveBlend() const { return m_moveBlend; }
