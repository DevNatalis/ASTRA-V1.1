#pragma once
#include <cstdlib>

namespace Core::Guard {

// Valores randomizados por sessao. Chamar Init() quando conectar
// em um servidor novo.
class SessionProfile {
public:
    static void Init() {
        aimFov_          = RandF(3.5f, 6.5f);
        aimSmooth_       = RandF(4.0f, 8.0f);
        noclipSpeed_     = RandF(2.5f, 4.0f);
        superJumpForce_  = RandF(8.0f, 11.0f);
        triggerDelay_    = RandI(45, 80);
        godModeHealTick_ = RandI(6, 12);
        espRefreshMs_    = RandI(40, 80);
    }

    static float AimFov()          { return aimFov_; }
    static float AimSmooth()       { return aimSmooth_; }
    static float NoclipSpeed()     { return noclipSpeed_; }
    static float SuperJumpForce()  { return superJumpForce_; }
    static int   TriggerDelay()    { return triggerDelay_; }
    static int   GodModeHealTick() { return godModeHealTick_; }
    static int   EspRefreshMs()    { return espRefreshMs_; }

private:
    static float aimFov_, aimSmooth_, noclipSpeed_, superJumpForce_;
    static int   triggerDelay_, godModeHealTick_, espRefreshMs_;

    static float RandF(float a, float b) {
        float t = (float)rand() / (float)RAND_MAX;
        return a + (b - a) * t;
    }
    static int RandI(int a, int b) {
        return a + (rand() % (b - a + 1));
    }
};

// Definicoes (uma vez por processo)
inline float SessionProfile::aimFov_          = 5.0f;
inline float SessionProfile::aimSmooth_       = 6.0f;
inline float SessionProfile::noclipSpeed_     = 3.0f;
inline float SessionProfile::superJumpForce_  = 9.5f;
inline int   SessionProfile::triggerDelay_    = 60;
inline int   SessionProfile::godModeHealTick_ = 8;
inline int   SessionProfile::espRefreshMs_    = 60;

} // namespace
