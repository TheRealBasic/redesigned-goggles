#pragma once

class Timer {
public:
    explicit Timer(double fixedDeltaSeconds = 1.0 / 60.0);
    void tick(double frameSeconds);
    bool canStep() const;
    void consumeStep();
    double alpha() const;
    double delta() const;

private:
    double m_fixedDelta;
    double m_accumulator;
};
