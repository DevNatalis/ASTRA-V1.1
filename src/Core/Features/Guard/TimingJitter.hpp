#pragma once
#include <Windows.h>
#include <cstdlib>

namespace Core::Guard {

class TimingJitter {
public:
    TimingJitter(int minMs = 25, int maxMs = 60)
        : min_(minMs), max_(maxMs) {
        next_ = Now() + RandRange();
    }

    // Retorna true em intervalos aleatorios entre min e max ms.
    bool Ready() {
        ULONGLONG now = Now();
        if (now < next_) return false;
        next_ = now + RandRange();
        return true;
    }

    // Forca o proximo Ready() a retornar true imediatamente.
    void ForceReady() { next_ = 0; }

    // Reconfigura a janela.
    void Configure(int minMs, int maxMs) {
        min_ = minMs; max_ = maxMs;
        next_ = Now() + RandRange();
    }

    // Consulta (nao consome).
    bool WouldBeReady() const { return Now() >= next_; }

private:
    int min_, max_;
    ULONGLONG next_ = 0;

    static ULONGLONG Now() { return GetTickCount64(); }

    ULONGLONG RandRange() {
        int range = max_ - min_;
        if (range < 1) range = 1;
        return (ULONGLONG)(min_ + (rand() % (range + 1)));
    }
};

} // namespace
