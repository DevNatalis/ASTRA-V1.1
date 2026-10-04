# Revisão estática de segurança — 03/10/2026

## Escopo e limites

Inspeção da estrutura e buscas transversais em src, restored-astra, Quantum com Login, ASTRA BKP, recovery e fivem-offset-dumper-main, com leitura direcionada dos fluxos de autenticação, rede, memória, configuração, clipboard e compilação. Não equivale a revisão manual de cada linha de todas as cópias e bibliotecas. Nenhum executável foi iniciado, nenhum driver foi carregado e nenhuma API de autenticação foi testada. Não foram alterados fontes. Não houve compilação, análise dinâmica, fuzzing ou auditoria do servidor e dos binários .lib/.exe. Gravidades são qualitativas; exploração não foi demonstrada.

## Achados

### 1. Alta, latente — capacidade incorreta na leitura do Registro

Local: src/Core/Threads/UpdateNames.hpp:73–76; também em restored-astra/src e cópias antigas.

`value` tem 255 bytes, mas `BufferSize` informa 8192 à API RegGetValue. Um valor longo em HKCU pode sobrescrever a pilha se essa rotina executar. A origem é modificável pelo usuário local; impacto adicional depende dos privilégios do processo. A inicialização da thread está comentada em src/Core/Core.hpp:38, portanto não foi constatada exposição no fluxo principal atual.

Correção: informar sizeof(value), inicializar o buffer e tratar ERROR_MORE_DATA, ou consultar o tamanho e alocar com limite. Não reativar a rotina antes de corrigir.

Referência: https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-reggetvaluea

### 2. Média — leituras fora dos limites em buscas de padrões

Locais: src/Core/SDK/Memory.cpp:60 e :111; fivem-offset-dumper-main/memory/memory.cpp:37.

As rotinas de src iteram até i < bytesRead, mas leem data[i+j], sem garantir espaço para toda a assinatura. Isso pode ler além do bloco alocado. No dumper, read_count - sig_len pode sofrer underflow quando o padrão exceder a quantidade lida. Os resultados possíveis incluem encerramento do processo e endereços incorretos; não foi demonstrada execução de código nem exfiltração.

Correção: validar tamanho não vazio e quantidade lida >= tamanho do padrão; limitar a última posição ao comprimento restante. Verificar também os retornos das APIs de memória.

### 3. Média — campos de autenticação enviados sem codificação

Local: src/Auth/auth_manager.hpp:349–369; implementação semelhante em Quantum com Login/src/Security/Api/api.hpp:387–445.

Usuário, senha e licença são concatenados diretamente em application/x-www-form-urlencoded. Caracteres como & e + alteram a interpretação dos campos, causando corrupção das credenciais e possível injeção de parâmetros. O impacto de parâmetros duplicados depende do servidor; não foi comprovado desvio de autenticação.

Correção: codificar cada valor individualmente. restored-astra/src/Auth/auth_manager.hpp já contém FormEncode e o utiliza nesses campos; a correção não está em src.

### 4. Média — compartilhamento sem sincronização do resultado do login

Locais: src/Auth/auth_manager.hpp:57–61, :130–138 e :176; src/Gui/Pages/Login.hpp:315.

Uma thread escreve ready/result/message e a interface lê ready sem sincronização. ready é bool comum. Isso constitui data race e comportamento indefinido, podendo causar resultados inconsistentes ou falhas. requestInProgress atômico não sincroniza uma leitura independente de ready.

Correção: publicar e consumir o resultado com mutex ou protocolo atômico consistente. A cópia restored-astra já declara ready como atomic<bool>; essa melhoria não está em src.

### 5. Média, condicional — validação de assinatura aceita dados incompletos

Locais: src/Auth/auth_manager.hpp:379–420 e restored-astra/src/Auth/auth_manager.hpp:443–484.

Depois de success=true, a ausência de info ainda resulta em Ok. expiry ausente assume zero, e erro na conversão é ignorado. Assim, a checagem local de assinatura permite respostas incompletas ou inválidas. Isso exige uma resposta aceita da API; não demonstra que um atacante de rede consiga falsificar TLS ou que o servidor permita acesso indevido.

Correção: exigir os campos e tipos esperados e rejeitar dados inválidos. Documentar explicitamente eventual convenção de assinatura vitalícia. A autorização efetiva deve permanecer no servidor.

### 6. Média — clipboard sem validação de ponteiro e tamanho

Local: src/Includes/Utils.hpp:149–156; chamado em src/Gui/Pages/Settings.hpp:130.

OpenClipboard, GetClipboardData e GlobalLock não têm o resultado validado antes de construir std::string a partir do ponteiro. Clipboard indisponível, sem CF_TEXT, ou conteúdo sem terminação adequada pode causar falha ou leitura além do bloco. Há necessidade de interação do usuário para importar a configuração.

Correção: validar cada chamada, limitar a leitura ao tamanho do objeto e garantir liberação dos recursos em todos os caminhos.

### 7. Média, latente — rede sem limites e descoberta por HTTP

Locais: src/Core/Threads/UpdateNames.hpp:37–40, :112–129 e :165–186.

A descoberta usa HTTP e aceita redirecionamentos. O callback acumula o corpo inteiro sem limite, e as requisições não configuram prazo máximo. Um servidor controlado ou interferência no trecho HTTP pode consumir memória ou bloquear a atualização. A thread está desativada no fluxo principal inspecionado. O callback de autenticação também não limita tamanho, embora a requisição tenha timeout de dez segundos e verificação TLS habilitada.

Correção: limitar corpo e duração, validar destino e protocolos dos redirecionamentos e usar transporte autenticado quando disponível. Tratar erros de JSON sem encerrar permanentemente a atualização.

### 8. Segredo aparente exposto — validade e impacto não verificados

Local: Quantum com Login/src/Security/Api/api.hpp:298; repetido em ASTRA BKP/Quantum com Login/src/Security/Api/api.hpp.

Existe um literal não vazio denominado secret. O valor não é reproduzido neste relatório. Não foi confirmado se ainda é válido, se é usado ou quais permissões possui. XOR de strings não preserva um segredo entregue no cliente. name e ownerid, isoladamente, não foram classificados como segredos administrativos.

Correção: verificar a natureza da credencial e revogá-la/rotacioná-la se ativa e confidencial; remover de clientes e artefatos distribuídos. Não presumir que a remoção do fonte invalida cópias anteriores.

## Dependências e componentes que exigem verificação adicional

- src/Security/Api/curl/curlver.h:33 declara libcurl 7.75.0. A tabela oficial relaciona vulnerabilidades para essa versão: https://curl.se/docs/vuln-7.75.0.html . A versão e as opções efetivas de libcurl.lib não foram confirmadas; não é correto atribuir automaticamente todas as CVEs ao executável. Confirmar procedência, versão compilada e funcionalidades, atualizar a dependência e reconstruir.
- Os cabeçalhos de FreeType declaram 2.12.1 e ImGui declara 1.90 WIP. Não houve confirmação de correspondência entre cabeçalhos e bibliotecas nem auditoria completa dessas dependências.
- src/Bypass/Manipulation/Bypass.cpp contém operações de memória de kernel via RTCore64 e referências a exploração de driver. Não foi encontrada chamada ativa de EnabledBypass nas buscas do fonte principal; a referência em Settings está comentada. Isso não comprova driver instalado ou vulnerabilidade ativa no computador. Remover componentes desse tipo da distribuição e confirmar que drivers vulneráveis não fazem parte dos artefatos entregues.
- CreateProcessAsUser usa applicationName nulo e GetCommandLine em src/Gui/Overlay/Overlay.cpp:538. Existe risco condicional de interpretação ambígua de caminho se a linha de comando não estiver corretamente delimitada. Não foi identificada chamada ativa de PrepareForUIAccess em src; Quantum contém chamada. Preferir caminho absoluto explícito do aplicativo. Referência: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessasusera
- Release x64 desabilita avisos em src/src.vcxproj:136. Habilitar avisos e análise estática. Ausência de flags explícitas no projeto não foi tratada como prova de ASLR/DEP desativados.

## Prioridade sugerida

1. Corrigir os limites de memória e validar o clipboard; manter a rotina de nomes desativada até corrigida.
2. Incorporar à versão realmente distribuída a codificação de formulários e a sincronização já presentes em restored-astra; revisar a validação da assinatura.
3. Verificar e revogar o segredo aparente, se aplicável; confirmar e atualizar as bibliotecas compiladas.
4. Consolidar qual árvore é a versão oficial e excluir cópias antigas do pacote de distribuição para impedir reintrodução das falhas.

Não há evidência suficiente nesta revisão para declarar o projeto livre de malware, afirmar comprometimento do computador ou garantir ausência de outras vulnerabilidades.
