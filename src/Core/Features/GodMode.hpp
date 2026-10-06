#pragma once
// GodMode anti-detect: restaura vida por limiar em vez de flag permanente.
// Adaptado de GodMode_AntiDetect.cpp (standalone) para a arquitetura ASTRA
// (usa Core::Mem + pLocalPlayer ja resolvido + throttle no render loop).
//
// Estrategia:
//   1. So escreve quando o health cai abaixo do limiar — zero IO saudavel.
//   2. Restore escalonado (55% / 40% critico), nunca instantaneo pra 100%.
//   3. Cooldown entre heals — sem padrao repetido visivel pro server.
//   4. Jitter aleatorio no valor — quebra matching exato.
//   5. MaxHealth antes de Health — nunca health > max num frame.
//   6. Flag nativa 0x189 opcional (default OFF).
//   7. Armor escrita antes de Health (menos suspeito).
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

#include <chrono>
#include <cstdlib>

namespace Core
{
	namespace Features
	{
		class cGodModeAntiDetect
		{
		public:
			// Tunaveis (mesmos defaults do script original).
			float DangerLow = 0.25f;       // age abaixo disso
			float DangerCritical = 0.10f;  // age sempre, sem cooldown
			float HealTarget = 0.55f;      // restaura ate aqui
			float HealTargetCrit = 0.40f;  // restore critico
			uint32_t MinTicksBetweenHeals = 8; // ~320ms entre curas (tick 40ms)
			float JitterRange = 0.06f;     // +/-6% no valor alvo
			bool UseNativeGodMode = false; // escrever 1 em 0x189
			bool RestoreArmor = true;
			bool RestoreStamina = true;
			uint32_t TickMs = 40;

			void SetEnabled(bool enabled)
			{
				enabled_ = enabled;
				if (!enabled)
					ClearFlags();
				else
				{
					SeedOnce();
					ticksSinceHeal_ = MinTicksBetweenHeals; // permite agir de imediato se ja estiver baixo
					lastHealth_ = 0.0f;
				}
			}

			bool IsEnabled() const { return enabled_; }

			void SetNativeGodMode(bool use)
			{
				UseNativeGodMode = use;
				if (!use && enabled_)
				{
					CPed* ped = SDK::Pointers::pLocalPlayer;
					if (ped)
						Mem.Write<BYTE>(reinterpret_cast<uintptr_t>(ped) + kGodNative, 0);
				}
			}

			// Chamado todo frame do render loop; faz throttle interno de 40ms.
			void Tick()
			{
				if (!enabled_)
					return;

				const auto now = std::chrono::steady_clock::now();
				const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick_).count();
				if (elapsed < static_cast<long long>(TickMs))
					return;
				lastTick_ = now;

				CPed* ped = SDK::Pointers::pLocalPlayer;
				if (!ped)
					return;
				const uintptr_t base = reinterpret_cast<uintptr_t>(ped);

				ticksSinceHeal_++;

				const uintptr_t maxOff = g_Offsets.m_MaxHealth ? g_Offsets.m_MaxHealth : kHealth + 4;
				const uintptr_t armorOff = g_Offsets.m_Armor ? g_Offsets.m_Armor : kArmorDefault;

				float health = Mem.Read<float>(base + kHealth);
				float maxHealth = Mem.Read<float>(base + maxOff);

				if (maxHealth <= 0.0f || maxHealth > kSanityMax)
					return;
				if (health < -1.0f || health > kSanityMax)
					return;

				const float ratio = health / maxHealth;
				lastHealth_ = health;

				if (UseNativeGodMode)
				{
					const BYTE gm = Mem.Read<BYTE>(base + kGodNative);
					if (gm != 1)
						Mem.Write<BYTE>(base + kGodNative, static_cast<BYTE>(1));
				}

				// Regra 1 — CRITICO: age sempre, sem cooldown.
				if (ratio < DangerCritical)
				{
					RestoreHealth(base, maxOff, health, maxHealth, HealTargetCrit);
					ticksSinceHeal_ = 0;
					return;
				}

				// Regra 2 — BAIXO + COOLDOWN.
				if (ratio < DangerLow && ticksSinceHeal_ >= MinTicksBetweenHeals)
				{
					RestoreHealth(base, maxOff, health, maxHealth, HealTarget);
					ticksSinceHeal_ = 0;
					return;
				}

				// Regra 3 — dano leve: aceita, nada a fazer (default).

				// Armor passiva (antes de health seria no restore; aqui e upkeep raro).
				if (RestoreArmor && ticksSinceHeal_ >= MinTicksBetweenHeals * 2)
				{
					const float armor = Mem.Read<float>(base + armorOff);
					if (armor >= 0.0f && armor < 15.0f)
					{
						float target = 35.0f + Jitter(10.0f);
						if (target < 0.0f)
							target = 0.0f;
						Mem.Write<float>(base + armorOff, target);
					}
				}

				// Stamina (via CPlayerInfo, como CPed::SetInfStamina).
				if (RestoreStamina && ticksSinceHeal_ >= MinTicksBetweenHeals)
				{
					CPlayerInfo* info = ped->GetPlayerInfo();
					if (info)
					{
						const uintptr_t infoBase = reinterpret_cast<uintptr_t>(info);
						const float stam = Mem.Read<float>(infoBase + kStamina);
						if (stam >= 0.0f && stam < 40.0f)
							Mem.Write<float>(infoBase + kStamina, 100.0f);
					}
				}
			}

			// Guarda pros teleports (TpToWaypoint / Teleportar): forca a flag
			// so durante a queda e depois devolve o controle pro Tick.
			void TeleportGuardBegin()
			{
				CPed* ped = SDK::Pointers::pLocalPlayer;
				if (ped)
					ped->SetGodMode(true);
				forcedFlag_ = true;
			}

			void TeleportGuardEnd(bool stealthEnabled)
			{
				CPed* ped = SDK::Pointers::pLocalPlayer;
				forcedFlag_ = false;
				if (!ped)
					return;
				if (stealthEnabled && enabled_)
				{
					// Limpa a flag forcada; o Tick cuida da vida por limiar.
					ped->SetGodMode(false);
					ticksSinceHeal_ = MinTicksBetweenHeals;
					lastHealth_ = 0.0f;
				}
				else
				{
					ped->SetGodMode(g_Config.Player->EnableGodMode);
				}
			}

		private:
			static constexpr uintptr_t kHealth = 0x280;
			static constexpr uintptr_t kGodFlag = 0x188;
			static constexpr uintptr_t kGodNative = 0x189;
			static constexpr uintptr_t kArmorDefault = 0x150C;
			static constexpr uintptr_t kStamina = 0xCF4;    // relativo a CPlayerInfo
			static constexpr float kSanityMax = 100000.0f;

			bool enabled_ = false;
			bool forcedFlag_ = false;
			uint32_t ticksSinceHeal_ = 0;
			float lastHealth_ = 0.0f;
			std::chrono::steady_clock::time_point lastTick_{};

			static void SeedOnce()
			{
				static bool seeded = false;
				if (!seeded)
				{
					seeded = true;
					srand(static_cast<unsigned>(time(nullptr)));
				}
			}

			static float Jitter(float range)
			{
				SeedOnce();
				const float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
				return (r * 2.0f - 1.0f) * range;
			}

			void RestoreHealth(uintptr_t ped, uintptr_t maxOff, float current, float maxHealth, float targetRatio)
			{
				// 1. Jitter no ratio — quebra matching exato.
				float ratio = targetRatio + Jitter(JitterRange);
				if (ratio < 0.05f)
					ratio = 0.05f;
				if (ratio > 0.95f)
					ratio = 0.95f;

				float target = maxHealth * ratio;

				// 2. Nunca passa do max.
				if (target > maxHealth)
					target = maxHealth;
				if (target < 1.0f)
					target = 1.0f;

				// 3. Nunca escreve valor menor que o atual.
				if (target <= current)
					return;

				// 4. ORDEM IMPORTA: MaxHealth primeiro (se mudou).
				const float curMax = Mem.Read<float>(ped + maxOff);
				if (curMax < maxHealth - 0.1f)
					Mem.Write<float>(ped + maxOff, maxHealth);

				// 5. Escreve health.
				Mem.Write<float>(ped + kHealth, target);
				lastHealth_ = target;
			}

			void ClearFlags()
			{
				CPed* ped = SDK::Pointers::pLocalPlayer;
				if (ped)
					ped->SetGodMode(false); // limpa bits 8/9 de 0x188
				if (ped)
					Mem.Write<BYTE>(reinterpret_cast<uintptr_t>(ped) + kGodNative, 0);
				ticksSinceHeal_ = 0;
				lastHealth_ = 0.0f;
				forcedFlag_ = false;
			}
		};

		inline cGodModeAntiDetect g_GodMode;
	}
}
