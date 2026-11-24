#pragma once
#include <algorithm>
#include <cmath>

// Velocity is in axis units/ms. Decay integrates elapsed time, not frame count.
struct Momentum {
    double velocity = 0;
    double age = 0;
    bool active = false;
    void reset() { velocity = age = 0; active = false; }
    void sample(double delta, double dt) {
        if (!std::isfinite(delta) || !std::isfinite(dt)) { reset(); return; }
        active = false;
        age = 0;
        const double next = std::clamp(delta / std::clamp(dt, 4.0, 50.0), -4.0, 4.0);
        velocity = velocity * next < 0 ? next : velocity * 0.5 + next * 0.5;
    }
    void release(double idle) { active = idle < 80 && std::abs(velocity) >= 0.02; }
    double tick(double dt) {
        if (!active || !std::isfinite(dt) || dt <= 0 || dt > 100 || age + dt > 800) { reset(); return 0; }
        const double decay = std::exp(-dt / 140.0);
        const double delta = velocity * 140.0 * (1.0 - decay);
        velocity *= decay;
        age += dt;
        if (std::abs(velocity) < 0.02) active = false;
        return delta;
    }
};
