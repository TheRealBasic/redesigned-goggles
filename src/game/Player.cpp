#include "game/Player.hpp"

#include "game/Map.hpp"

#include <cmath>

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
}

float Player::x() const { return m_x; }
float Player::y() const { return m_y; }
