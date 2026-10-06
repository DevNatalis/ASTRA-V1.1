#pragma once
// Force weapon wheel (unlock wheel): mantem a roda de arma disponivel mesmo
// quando o servidor bloqueia via DisableControlAction.
//
// A cada tick resolve o CPed do local player pela cadeia World -> +0x8
// (CPedFactory::GetLocalPlayer) e delega ao CPed::ForceWeaponWheel, que
// localiza o bitmap de controles desabilitados no CPed (range +0x1000 a
// +0x1800, janela de 48 bytes identificada pelo controle 37) e zera os bits
// 37/157/158/24/25 somente se algum estiver setado (cooldown = zero IO
// quando ja esta liberado). Se o ped mudar, o scan e refeito.
//
// Offsets base (build b3258): World = ModBase + 0x25B14B0,
// ReplayInterface = ModBase + 0x1FBD4F0, Ped_Health = 0x280. Aqui eles vêm
// de g_Offsets (resolvidos por assinatura para cada build em Core.hpp),
// com o pLocalPlayer ja resolvido como fallback.
//
// Acesso de leitura/escrita: usa o driver de memoria do projeto
// (Core::Mem via <Core/SDK/SDK.hpp>); nao requer driver adicional.
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

#include <chrono>

namespace Core
{
	namespace Features
	{
		class cWeaponWheel
		{
		public:
			void SetEnabled(bool enabled)
			{
				enabled_ = enabled;
				if (!enabled) {
					if (CPed* ped = ResolveLocalPed())
						ped->ForceWeaponWheel(false); // invalida o cache do bitmap
				}
				else {
					lastTick_ = std::chrono::steady_clock::time_point{};
				}
			}

			bool IsEnabled() const { return enabled_; }

			// Força a roda pra disponivel: resolve o ped via World -> +0x8
			// e zera os bits de controle no bitmap do CPed.
			void ForceWeaponWheel()
			{
				CPed* ped = ResolveLocalPed();
				if (!ped) return;
				ped->ForceWeaponWheel(true);
			}

			// Chamado todo frame do render loop; throttle interno de 30ms.
			void Tick()
			{
				if (!enabled_) return;

				const auto now = std::chrono::steady_clock::now();
				if (lastTick_ != std::chrono::steady_clock::time_point{} &&
					std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick_).count() < static_cast<long long>(TickMs))
					return;
				lastTick_ = now;

				ForceWeaponWheel();
			}

		private:
			static constexpr uint32_t TickMs = 30;
			static constexpr float SanityMax = 100000.0f;

			bool enabled_ = false;
			std::chrono::steady_clock::time_point lastTick_{};

			static CPed* ResolveLocalPed()
			{
				// Primaria: World -> CPedFactory -> +0x8.
				if (g_Offsets.m_World) {
					CPedFactory* factory = Mem.Read<CPedFactory*>(g_Offsets.m_World);
					if (factory) {
						CPed* ped = factory->GetLocalPlayer();
						if (ped && LooksAlive(ped)) return ped;
					}
				}
				// Fallback: ponteiro ja resolvido pela thread de pointers.
				if (CPed* cached = SDK::Pointers::pLocalPlayer) {
					if (LooksAlive(cached)) return cached;
				}
				return nullptr;
			}

			static bool LooksAlive(CPed* ped)
			{
				if (!ped) return false;
				const float h = Mem.Read<float>(reinterpret_cast<uintptr_t>(ped) + 0x280);
				return h > 0.0f && h < SanityMax;
			}
		};

		inline cWeaponWheel g_WeaponWheel;
	}
}
