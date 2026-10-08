#pragma once
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/SDK/Guard.hpp>
#include <Core/Features/Guard/FeatureGuard.hpp>
#include <Core/Offsets.hpp>
#include <Core/Core.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>

namespace Core
{
    namespace Threads
    {
        class cUpdatePtrs {
        public:
            void Update( )
            {
                while ( true )
                {
                    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

                     //   continue;

                    // STABILITY: each pointer resolves independently; one bad
                    // offset/handle must not blank or crash the others.
                    if (!::Guard::ProcReady()) {
                        Debug::Warning("UpdatePtrs::Update", "proc not ready, retry later");
                        continue;
                    }

                    if (::Guard::IsOffset(g_Offsets.m_World)) {
                        auto* world = Mem.Read<CPedFactory*>(g_Offsets.m_World);
                        Core::SDK::Pointers::pWorld = world;
                        // STABILITY (causa concreta): pWorld era dereferenciado sem
                        // null-check -> AV local. Agora so resolve o player se valido.
                        if (world) {
                            CPed* fresh = world->GetLocalPlayer();
                            // FeatureGuard: transicao null -> valido indica (re)conexao
                            // com a sessao: re-randomiza os valores por sessao para
                            // consistencia entre reconexoes.
                            static bool hadPlayer = false;
                            if (fresh && !hadPlayer)
                                Core::Guard::OnSessionStart();
                            hadPlayer = (fresh != nullptr);
                            Core::SDK::Pointers::pLocalPlayer = fresh;
                        }
                        else
                            Debug::Warning("UpdatePtrs::Update", "pWorld null, keep old player");
                    }
                    else {
                        Debug::Warning("UpdatePtrs::Update", "m_World offset 0, skip");
                    }

                    if (::Guard::IsOffset(g_Offsets.m_ReplayInterFace))
                        Core::SDK::Pointers::pReplayInterFace = Mem.Read<CReplayInterFace*>(g_Offsets.m_ReplayInterFace);
                    else
                        Debug::Warning("UpdatePtrs::Update", "m_ReplayInterFace offset 0, skip");

                    if (::Guard::IsOffset(g_Offsets.m_ViewPort))
                        Core::SDK::Pointers::pViewPort = Mem.Read<uintptr_t>(g_Offsets.m_ViewPort);

                    if (::Guard::IsOffset(g_Offsets.m_CamGameplayDirector))
                        Core::SDK::Pointers::pCamGamePlayDirector = Mem.Read<uintptr_t>(g_Offsets.m_CamGameplayDirector);
                }
            }
        };

        inline cUpdatePtrs g_UpdatePtrs;
    }
}
