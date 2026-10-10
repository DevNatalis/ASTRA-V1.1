#pragma once
// Reaplica toggles ativos a cada 500ms: alguns servidores
// redefinem flags do ped, e a aplicacao unica do clique se perde.
// Reaplicar mantem o estado pedido pelo usuario.
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

#include <chrono>

namespace Core
{
	namespace Features
	{
		class cStickyToggles
		{
		public:
			void Tick()
			{
				CPed* ped = SDK::Pointers::pLocalPlayer;
				if (!ped)
					return;

				// Flags (leitura + escrita condicional): reaplicacao
				// imediata, todo frame. O jogo/servidor limpa essas
				// flags em eventos (dano, veiculo, sync); reaplicar
				// no frame seguinte nao da janela para o revert.
				if (g_Config.Player->AntiHSEnabled)
					ped->SetConfigFlag(ePedConfigFlag::NoCriticalHits, true);
				if (g_Config.Player->ShrinkEnabled)
					ped->SetConfigFlag(ePedConfigFlag::Shrink, true);
				if (g_Config.Player->StealCarEnabled) {
					ped->SetConfigFlag(ePedConfigFlag::NotAllowedToJackAnyPlayers, false);
					ped->SetConfigFlag(ePedConfigFlag::PlayerCanJackFriendlyPlayers, true);
					ped->SetConfigFlag(ePedConfigFlag::WillJackAnyPlayer, true);
				}

				const auto now = std::chrono::steady_clock::now();
				if (lastTick_ != std::chrono::steady_clock::time_point{} &&
					std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick_).count() < kTickMs)
					return;
				lastTick_ = now;

				// Escritas incondicionais: cadencia de 500ms.
				if (g_Config.Player->NoRagDollEnabled)
					ped->NoRagDoll(true);
				if (g_Config.Player->InfiniteStamina)
					ped->SetInfStamina(true);
				if (g_Config.Player->FastRun)
					ped->SetSpeed(g_Config.Player->RunSpeed);
				if (g_Config.Player->InfiniteCombatRoll && g_Offsets.m_InfiniteCombatRoll)
					ped->SetInfCombatRoll(true);
			}

		private:
			static constexpr long long kTickMs = 500;
			std::chrono::steady_clock::time_point lastTick_{};
		};

		inline cStickyToggles g_StickyToggles;
	}
}
