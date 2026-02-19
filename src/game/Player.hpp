#pragma once

struct InputState {
    bool up;
    bool down;
    bool left;
    bool right;
};

class Map;

class Player {
public:
    void setPosition(float x, float y);
    void update(const InputState& input, const Map& map, float dtSeconds);

    float x() const;
    float y() const;

private:
    float m_x = 2.0F;
    float m_y = 2.0F;
    float m_speed = 4.0F;
};
