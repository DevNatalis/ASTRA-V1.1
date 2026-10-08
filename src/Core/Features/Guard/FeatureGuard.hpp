#pragma once
#include "ValueSmoother.hpp"
#include "TimingJitter.hpp"
#include "SessionProfile.hpp"

namespace Core::Guard {

// Ponto central da escrita das features em memoria: interpolacao
// gradual (ValueSmoother) + distribuicao temporal (TimingJitter),
// com valores por sessao (SessionProfile). Melhora estabilidade e
// evita condicoes de corrida com o runtime do jogo.

// Atalho pra chamar SessionProfile::Init() no connect do server.
inline void OnSessionStart() {
    SessionProfile::Init();
}

} // namespace
