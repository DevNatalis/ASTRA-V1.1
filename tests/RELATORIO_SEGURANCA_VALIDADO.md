# Resultado geral

Revisão de 06/10/2026, usando exclusivamente `RELATORIO_SEGURANCA.md` como checklist dos oito achados originais. O relatório original foi preservado.

**8 achados originais: 4 corrigidos nesta revisão, 3 já estavam corrigidos, 1 não se aplica à build atual. Nenhum dos sete itens aplicáveis permanece parcialmente corrigido.**

Árvore oficial identificada antes das alterações: `src/src.slnx:6` referencia `src/src.vcxproj`; esse projeto compila os fontes de `src` e gera `src/x64/Release/ASTRA.exe`. O dumper é uma ferramenta separada e também recebeu a correção do item 2. Não existem neste workspace as árvores `restored-astra`, `Quantum com Login`, `ASTRA BKP` ou `recovery` citadas em 03/10. As alterações preexistentes foram preservadas; nenhuma funcionalidade de gameplay foi modificada nesta revisão.

Release e Debug x64 passaram sem erros após a atualização das dependências e da seleção das bibliotecas DirectX. `dist` e o ZIP foram atualizados com o Release validado. Nenhum executável ASTRA ou driver foi iniciado; somente testes isolados foram executados.

# Achados originais

## 1. Registro

**Status:** JÁ CORRIGIDO, antes e depois da revisão.

**Arquivos:** `src/Core/Threads/UpdateNames.hpp:104`, função `cUpdateNames::GetServerToken`; leitura em aproximadamente 110–118.

**Problema encontrado:** o erro descrito no relatório não permanece: `char value[255] = {}` possui 255 bytes e `DWORD BufferSize = sizeof(value)` informa exatamente essa capacidade a `RegGetValueA`.

**Correção aplicada:** nenhuma no trecho de Registro. Qualquer retorno diferente de `ERROR_SUCCESS`, inclusive `ERROR_MORE_DATA`, interrompe o processamento antes de construir a string.

**Evidência:** releitura pós-correção e testes com comprimentos exigidos de 255, 256 e 8192 bytes. Para `REG_SZ`, a API acrescenta terminador quando necessário ou retorna `ERROR_MORE_DATA`; ver [contrato de RegGetValueA](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-reggetvaluea). A busca global encontrou outra leitura em `src/Bypass/Manipulation/Bypass.cpp:209`, `getVersionOffsets`, também com `sizeof(value)`; esse arquivo foi excluído da compilação.

**Risco residual:** caminhos maiores que a capacidade são rejeitados, sem suporte novo a caminhos longos. Diferentemente do relatório antigo, `Core::StartThreads`, em `src/Core/Core.hpp:38`, inicia a thread de nomes na árvore atual.

## 2. Pattern scanning

**Status:** PROCEDE inicialmente; CORRIGIDO.

**Arquivos:** `src/Core/SDK/Memory.cpp:22`, `MemoryClass::FindSignature`; `:82`, `FindSignatureBypass`; `:10`, `PatternScan`; `fivem-offset-dumper-main/memory/memory.cpp:32`, `find_in_region`, e `pattern_scan` em aproximadamente 55.

**Problema encontrado:** os dois scanners de `src` comparavam `data[i+j]` até `i < bytesRead`. O dumper calculava `read_count - sig_len` sem exigir `read_count >= sig_len`.

**Correção aplicada:** rejeição de padrão vazio; no scanner em blocos, padrões maiores que 16 KiB são rejeitados. Cada leitura respeita o fim do módulo, valida o retorno e o tamanho informado, exige bytes suficientes antes da subtração e limita o início a `bytesRead - signatureSize`. Há sobreposição entre blocos para padrões na fronteira. Foram protegidas somas de endereços e progressão do cursor; a proteção de página original é restaurada com ponteiro de saída válido. `PatternScan` não resolve uma assinatura inexistente.

**Evidência:** testes extraem e compilam as implementações atuais com APIs de memória simuladas. Passaram: padrão vazio, grande demais, leitura curta, leitura falha, última posição, wildcard final, fronteira entre blocos e overflow de endereço. O dumper testa especialmente `read_count < sig_len`. A busca pós-correção ainda encontra `data[i+j]` e a subtração, agora sob as guardas matemáticas necessárias.

**Risco residual:** não foi executada leitura de processo externo. Isso valida limites locais, não a validade de offsets de jogo ou de todas as funções de memória do projeto.

## 3. Form encoding

**Status:** JÁ CORRIGIDO na implementação ativa.

**Arquivos:** `src/Auth/auth_policy.hpp:7`, `AuthPolicy::FormEncode`; `src/Auth/auth_manager.hpp:834`, `BuildInitFields`; `:841`, `BuildLoginFields`; `:851`, `BuildRegisterFields`; `:948`, novo `BuildCheckFields`.

**Problema encontrado:** nenhum valor dinâmico cru nas requisições de formulário ativas. A integração que faltava no relatório antigo já existia em `src`.

**Correção aplicada:** preservada a implementação existente; o novo heartbeat também codifica sessão, nome e owner individualmente.

**Evidência:** testes para `&`, `+`, `=`, `%`, espaços e bytes UTF-8. A busca global encontrou apenas `PerformRequest` como transporte de formulário ativo. A API legada, que usava JSON e não tinha chamadas, foi removida.

**Risco residual:** codificação preserva os parâmetros; não substitui validação nem autorização do servidor.

## 4. Login assíncrono

**Status:** JÁ CORRIGIDO para o achado original.

**Arquivos:** `src/Auth/auth_manager.hpp:66`, `AsyncAuthResult`; `:231`, `LoginAsync`; `:259`, `RegisterAsync`; `:290`, `ConsumeResult`; `CanAttempt` em aproximadamente 1144; `src/Gui/Pages/Login.hpp:155`, `Login::Render`.

**Problema encontrado:** `ready` já era atômico, e a ordem das operações implementava um protocolo completo, não apenas um atomic isolado.

**Correção aplicada:** acrescentada explicação do protocolo, preservando as operações `seq_cst`. Um problema distinto na leitura de `page` por threads foi corrigido e está nos achados adicionais.

**Evidência:** o worker escreve `result/message`, publica `ready=true` e só depois publica `requestInProgress=false`. A UI exige `ready` e ausência de request antes de ler os campos. Os loads `seq_cst` têm semântica acquire, e os stores têm semântica release, estabelecendo happens-before. `CanAttempt` impede outro worker enquanto houver request ou resultado pendente; antes de reutilizar a thread ocorre `join`. O consumidor é a render thread. Passaram 500 publicações concorrentes exercitando o consumidor real, além da recusa de consumir com worker ocupado.

**Risco residual:** esse teste não é ThreadSanitizer nem prova ausência de outras races. O protocolo pressupõe um consumidor na UI e que os novos chamadores respeitem as mesmas guardas.

## 5. Resposta de assinatura

**Status:** PARCIALMENTE CORRIGIDO inicialmente; CORRIGIDO.

**Arquivos:** `src/Auth/auth_policy.hpp:25`, `AuthPolicy::ParseLicense`; `src/Auth/auth_manager.hpp:864`, `DoLogin`, e aproximadamente 903, `DoRegister`.

**Problema encontrado:** ausência de `info`, assinatura e expiry já era rejeitada. Entretanto, o parser retornava ao encontrar uma assinatura válida, sem verificar entradas seguintes; também ignorava entradas malformadas antes de uma válida.

**Correção aplicada:** todas as entradas precisam ter esquema e expiry válidos. O resultado só é publicado após validar o array completo e encontrar ao menos uma assinatura futura. Timestamp deve ser string decimal positiva, completamente convertível para `int64_t`. Expiry zero não significa licença vitalícia; não há convenção implícita de licença vitalícia. Uma assinatura expirada bem formada pode coexistir com outra vigente.

**Evidência:** testes rejeitam ausência, zero, negativos, vencimento, overflow, sufixos, null, bool, número JSON, array, objeto e entrada malformada depois de uma válida. `DoRegister` só concede resultado de autenticação através de `DoLogin`; `DoLogin` verifica novamente expiração antes de `Ok`.

**Risco residual:** autorização efetiva permanece no servidor. Mudança futura do esquema ou suporte a licença vitalícia exige contrato explícito; tipos inesperados falham fechados.

## 6. Clipboard

**Status:** PARCIALMENTE CORRIGIDO inicialmente; CORRIGIDO.

**Arquivos:** `src/Includes/Utils.hpp:150`, `Utils::GetClipboard`; chamador em `src/Gui/Pages/Settings.hpp`, `Settings::Render`.

**Problema encontrado:** já existiam verificações de `OpenClipboard`, `GetClipboardData`, `GlobalSize`, `GlobalLock`, terminador e guards para liberação. Faltava tamanho máximo aceitável.

**Correção aplicada:** blocos acima de 1 MiB são rejeitados antes de bloquear/copiar. A leitura continua limitada pela capacidade real. Os writers e o backend Unicode encontrados na busca de resíduos foram corrigidos separadamente.

**Evidência:** 10 casos de Registro/clipboard passaram, incluindo clipboard indisponível, handle/lock inválidos, ausência de terminador, conteúdo válido e capacidade excessiva. Guards liberam o lock e fecham o clipboard nos retornos e durante exceções de construção de string.

**Risco residual:** mantém CF_TEXT para compatibilidade das configurações; não foi alterado o formato de importação.

## 7. Rede e descoberta HTTP

**Status:** CORRIGIDO.

**Arquivos:** `src/Core/Threads/UpdateNames.hpp:51`, `WriteCallBack`; `:63`, `ConfigureRequest`; `:104`, `GetServerToken`; `:210`, `GetPlayerData`; `src/Auth/auth_manager.hpp:770`, `PerformRequest`, e `:821`, `WriteCallback`.

**Problema encontrado:** descoberta e consulta de nomes sem teto de corpo ou prazos; reutilização potencial de URL antiga. O callback de autenticação já tinha teto de 256 KiB.

**Correção aplicada:** nomes limitados a 2 MiB, multiplicação/subtração protegidas, callback sem propagação de exceções, prazo total de 10 s e conexão de 3 s. Descoberta exige HTTPS, desativa redirects automáticos e valida manualmente até três destinos antes de cada conexão. A API de jogadores exige HTTPS e não segue redirects. Token só é aceito sob `https://cfx.re/join/` ou `https://servers.fivem.net/servers/detail/`, com formato alfanumérico limitado. Falha de transferência/status não é processada como JSON válido. IP/URL anteriores são limpos; headers curl são liberados. Na autenticação, o teto de 256 KiB foi preservado e falhas de configuração curl, inclusive TLS/pin, abortam antes do request.

**Evidência:** reabertura dos callbacks e configurações, testes no tamanho exato do teto, um byte excedente, multiplicação excessiva e ponteiro nulo. JSON de nomes continua com verificação de objeto/array e captura de erros. A autenticação mantém TLS peer=1, host=2, HTTPS, redirects desativados e o pin original.

**Risco residual:** servidores somente HTTP ou sem certificado válido deixam de fornecer metadados de nomes. Nenhum servidor de jogo real foi usado na validação. `ConfigureRequest:64` desativa redirects automáticos; `IsDiscoveryDestination:80` e `GetServerToken:114` validam destino antes do próximo request, mantendo prazo único de dez segundos. Testes extraídos da função rejeitam HTTP, domínio semelhante, userinfo e caminho não autorizado. Os cinco testes TLS isolados passaram com a biblioteca efetivamente linkada.

## 8. Segredo embutido

**Status:** NÃO SE APLICA À BUILD ATUAL para o literal original; validade histórica não verificável neste workspace.

**Arquivos:** o caminho original `Quantum com Login/src/Security/Api/api.hpp:298` não existe aqui. `src/Auth/auth_manager.hpp:78`, `AuthSecrets`, contém identificadores públicos e URL. `src/Security/Api/api.hpp:1` ficou apenas como include de compatibilidade.

**Problema encontrado:** a busca por identificadores de segredo/chaves não encontrou o literal original no fonte disponível. A API legada tinha strings de autorização/criptografia vazias, sem endpoint e sem chamadas. Isso não prova revogação ou ausência em versões historicamente distribuídas.

**Correção aplicada:** removido código legado sem uso; nenhum segredo real foi substituído por ofuscação. Nenhuma credencial ou valor de segredo é reproduzido neste relatório. Nenhuma rotação foi executada.

**Evidência:** inventário dos fontes disponíveis, buscas globais e ausência de referências ao cliente legado. ZIP atualizado contém oito arquivos, nenhum `.sys` ou fonte dessas árvores. Não houve consulta ao painel ou tentativa de usar credenciais.

**Risco residual:** verificar no serviço correspondente se o segredo histórico era confidencial e continua ativo. Se distribuído e válido, revogar/rotacionar no serviço; apagar fonte não invalida cópias antigas. Identificadores públicos de aplicação não foram classificados como credenciais administrativas.

# Verificações adicionais

- **libcurl:** headers e biblioteca linkada agora são **8.22.0, Schannel**, confirmado por `curl_version_info` em `tests/security_report_checks.py`. Fontes do [release oficial](https://curl.se/download.html) foram recompilados com v143 x64; SHA-256 e opções estão em `scripts/build_security_dependencies.py` e `scripts/SECURITY_DEPENDENCIES.md`. Bibliotecas antigas permanecem somente no backup ignorado `build/security-update/before`. Não foram atribuídas CVEs pelo header.
- **FreeType:** mantida **2.12.1**, header e `FT_Library_Version` concordam. Bibliotecas recompiladas do tag oficial com /MD e /MDd; opções externas de compressão/PNG/HarfBuzz desativadas. Isso corrige o conflito de runtime sem afirmar atualização da versão do FreeType.
- **ImGui:** `imgui.h:26` declara **1.90 WIP**, número 18993. `ImGui::GetVersion`, `imgui.cpp:4262`, retorna essa macro; o `.vcxproj` compila esses fontes diretamente. Não há uma biblioteca ImGui separada no link. É uma árvore customizada, agora com correção local de clipboard.
- **RTCore64:** `src/Bypass/Manipulation/Bypass.cpp:173` abre o dispositivo; `EnabledBypass:242` não tinha chamada ativa. As três unidades `Bypass.cpp`, `reg.cpp` e `task.cpp` foram retiradas de `ClCompile`, e os includes sem uso saíram de Settings. O código-fonte permanece como resíduo não compilado; recomenda-se removê-lo de pacotes de fonte do produto. Nenhum arquivo `.sys` foi encontrado no workspace ou como entrada do ZIP. Isso não equivale a auditar drivers instalados no computador ou provar ausência de qualquer payload embutido.
- **CreateProcessAsUser:** `Overlay::PrepareForUIAccess`, `src/Gui/Overlay/Overlay.cpp:528`, agora usa `GetModuleFileNameW`, caminho explícito em `lpApplicationName`, `CreateProcessAsUserW` e cópia gravável da linha de comando. Preserva os argumentos originais; a seleção do executável não depende mais do quoting do primeiro token. Nenhuma chamada ativa de `PrepareForUIAccess` foi encontrada. Nenhum processo foi criado para testar essa rotina.
- **Configuração do build:** `src/src.vcxproj:141` mudou Release x64 de warnings desligados para `/W3`; `/sdl` permanece e `/GS` foi explicitado. Removida supressão global de LNK4099. `/analyze` foi realmente executado nesta validação, via `RunCodeAnalysis=true`; não está permanentemente habilitado por padrão. Inspeção do PE Release confirmou ASLR, DEP/NX e HighEntropyVA; não foi inferido seu estado pela ausência de flags no projeto.

# Achados adicionais

- **Sessão client-side:** `AuthManager::RequireAuth:404`, `IsSessionValid:390` e recibos locais podem ser substituídos por patch em um cliente controlado pelo usuário. São verificações locais, não uma raiz de confiança. Autorização de recursos críticos, rate limiting e banimento precisam ser aplicados pelo servidor. Não foi auditado código do servidor.
- **Estado compartilhado fora do item 4:** `IsSessionValid` lia `page` sob mutex enquanto a UI escrevia sem esse mutex. Removida essa leitura nos hot paths; eles consultam `armed/revoked/expiry/hbFails`, protegidos pelo mutex da sessão. Isso é distinto de `async.result/message`, cujo protocolo já estava correto.
- **Flags e API antigas:** `g_VerifyLogin` e `g_bPassedByThisVerify` eram escritas, sem leituras efetivas. Removidas declarações, atribuições e comentários antigos em Globals, Login e threads; o único cliente legado que também escrevia essas flags foi removido. Busca pós-correção não encontrou resíduos desses identificadores nos fontes oficiais.
- **SSL pinning:** `PerformRequest:770` continua com peer=1, host=2 e `CURLOPT_PINNEDPUBLICKEY`. Falha de setopt agora aborta. Comparação com a baseline confirmou pin inalterado. Não foi validada sua correspondência com o certificado atualmente servido. Rotação legítima de chave requer verificação independente e atualização planejada; não deve levar à desativação de TLS.
- **HWID:** `ReadSmbiosBlob:646`, `ReadDiskSerial:658`, `ComputeHwid:689` e `GetHwid:727` usam SMBIOS/disco, depois volume e, como último fallback explícito, SID prefixado. SID não é identidade forte de máquina. Corrigidos comprimento retornado de SMBIOS, string de serial sem terminador limitado e cache mutável sem mutex. A função SID-only duplicada `Utils::GetHWID`, sem chamadas, foi removida. Hash do blob SMBIOS pode mudar após atualização de firmware; não foi alterado o formato de HWID existente.
- **Heartbeat:** `HeartbeatLoop:1093` fazia `init`, que apenas comprovava disponibilidade. Passou a enviar `type=check` com sessão real codificada (`BuildCheckFields:948`), conforme [Check Session do serviço](https://keyauthdocs.apidog.io/api/features/check-session). Mantém intervalo de 12 minutos e timeout de 5 s; resposta explícita `success=false` revoga na checagem, falhas de transporte/parse revogam após três tentativas. Expiração da assinatura continua local e independente. Não há garantia de que ban posterior revogue sessões no serviço: isso precisa ser confirmado no servidor/painel. Também não há prazo separado de freshness aplicado a `lastHeartbeat` se a thread parar; a expiração de assinatura continua sendo o limite local.
- **Anti-debug/AntiCrack:** `src/Security/AntiCrack.hpp:1` está comentado. Não foram ativados detectores nem técnicas agressivas. Não se considera anti-debug uma barreira forte de autorização.
- **Clipboard adicional:** `Utils::PasteClipboard:172` e `Settings::CopyToClipboard:13` tinham falhas de validação de alocação/lock/transferência; corrigidas e centralizadas. `GetClipboardTextFn_DefaultImpl`, `imgui.cpp:13866`, usava conversão Unicode com comprimento `-1` sem limitar ao bloco: agora valida tamanho até 1 MiB e terminador antes da conversão. O setter em aproximadamente 13906 valida lock/conversão. Nove testes isolados do reader Unicode verificaram rejeição e liberação de recursos.
- **Avisos de análise estática:** existem avisos adicionais, inclusive C26115 no heartbeat, C26110/C26117 no render, C6385/C6386 em desenho ImGui e C6011/C6387 em caminhos de UI. Permanecem registrados para triagem, sem classificá-los automaticamente como vulnerabilidades exploráveis. O heartbeat utiliza `lock_guard` em escopos; o aviso sozinho não prova lock perdido.

# Arquivos modificados

- `src/Core/SDK/Memory.cpp`: limites, leituras, sobreposição e restauração de proteção dos scanners.
- `fivem-offset-dumper-main/memory/memory.cpp`: elimina underflow e protege progressão de regiões; ferramenta separada.
- `src/Core/Threads/UpdateNames.hpp`: limites/prazos/protocolos/status de rede, limpeza de estado e resíduos.
- `src/Auth/auth_policy.hpp`: validação completa das entradas de assinatura.
- `src/Auth/auth_manager.hpp`: falha fechada de configuração TLS, heartbeat de sessão, limites de HWID/cache e remoção da leitura concorrente de page; comentário do protocolo async.
- `src/Includes/Utils.hpp`: teto de clipboard, writer seguro e remoção de HWID duplicado sem uso.
- `src/Includes/ImGui/Files/imgui.cpp`: validação do clipboard Win32 Unicode.
- `src/Security/Api/api.hpp`: remove cliente legado sem uso, mantendo header de compatibilidade.
- `src/Globals.hpp`: remove duas flags de autenticação sem leituras.
- `src/Gui/Pages/Login.hpp`: remove atribuições dessas flags.
- `src/Core/Threads/UpdatePointers.hpp`: remove comentário de verificação antiga.
- `src/Core/Threads/VehicleList.hpp`: remove comentário de verificação antiga.
- `src/Gui/Pages/Settings.hpp`: centraliza writer de clipboard e remove includes de componentes não usados.
- `src/Gui/Overlay/Overlay.cpp`: caminho explícito do executável e remove include da API legada.
- `src/src.vcxproj`: `/W3`, `/GS`, retira supressão de linker e fontes de manipulação sem chamadas.
- `tests/auth_security_checks.cpp`: tipos inválidos, array misto, callback, heartbeat codificado e publicação concorrente.
- `tests/crash_regression_checks.py`: caso adicional de clipboard excessivo.
- `tests/security_report_checks.py`: testes offline dos scanners/callback e identificação das bibliotecas linkadas.
- `tests/clipboard_backend_checks.py`: testes offline do clipboard Unicode.
- `tests/RELATORIO_SEGURANCA_VALIDADO.md`: este relatório.

Evidências geradas em `build/security-audit`: logs completos, `evidence.json`, `residues.txt` e `security.diff` com mudanças desta revisão, sem misturar alterações preexistentes. A pasta `before` é uma baseline de auditoria, não árvore oficial nem correção distribuída.

# Build

 - **Release x64:** sucesso, 0 erros e 48 avisos; saída `src/x64/Release/ASTRA.exe`. A limpeza removeu 187 avisos, preservando comportamento: ajustes de tipos, constantes `float`, chamadas de UI, seleção de biblioteca de ícones e semântica de setters.
 - **Debug x64:** sucesso, 0 erros após a limpeza. A configuração usa bibliotecas curl/FreeType Debug e deixa o pacote DirectX selecionar suas bibliotecas Debug. Como o build foi incremental, o log final registra apenas os 24 avisos das unidades recompiladas; ele não representa uma contagem total do perfil Debug.
 - **Testes isolados:** scanner/callback, autenticação, clipboard e cinco casos TLS passaram. Os casos TLS rejeitaram HTTP, certificado não confiável, hostname incorreto e pin incorreto; aceitaram apenas o certificado local confiável com pin correto.
 - **Distribuição:** `dist/ASTRA.exe` e `ASTRA-v1.1-win64.zip` foram refeitos. O ZIP contém oito arquivos, passou em `testzip()` e o executável dentro do pacote tem o mesmo SHA-256 do Release: `577939a62d8f8864f321483ada9327005a723b8e6f3488d270e583430995afcf`.

# Pendências

1. Triar os 48 avisos Release restantes: conversões numéricas explícitas ainda faltantes em widgets customizados, preview, resolução de janela e animações de UI. Os conflitos Font Awesome e instruções controladas vazias foram eliminados. Os avisos restantes não foram silenciados nem classificados automaticamente como vulnerabilidades exploráveis.
2. Confirmar no serviço a natureza/validade da credencial histórica e rotacioná-la se ativa e confidencial. Ausência no fonte não prova revogação.
3. Validar em ambiente autorizado o check de sessão real, a política de revogação após ban e a autorização de recursos críticos no servidor. O pin de produção foi preservado, mas sua correspondência com o serviço atual não foi testada. Não há auditoria do backend neste workspace.
