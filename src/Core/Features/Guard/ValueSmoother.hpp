#pragma once
#include <cmath>
#include <cstdlib>

namespace Core::Guard {

class ValueSmoother {
public:
    struct Config {
        int   ticks     = 6;
        float jitterPct = 0.15f;
        float deadzone  = 0.5f;
    };

    // Interpola current -> target em N ticks com jitter.
    // Chame a cada tick de feature. Retorna proximo valor a escrever.
    static float Next(float current, float target, int& tick,
                     const Config& cfg = {})
    {
        float diff = target - current;
        if (std::fabs(diff) < cfg.deadzone) { tick = 0; return target; }

        float step   = diff / (float)cfg.ticks;
        float jitter = ((rand() % 200 - 100) / 100.0f) * cfg.jitterPct;
        float delta  = step * (1.0f + jitter);

        float next = current + delta;
        if ((step > 0 && next > target) || (step < 0 && next < target))
            next = target;

        ++tick;
        return next;
    }

    static void Reset(int& tick) { tick = 0; }
};

} // namespace
