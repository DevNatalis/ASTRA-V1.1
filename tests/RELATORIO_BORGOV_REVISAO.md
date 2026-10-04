# Revisão do relatório BORGOV

Revisão do código local em 2026-10-04. Esta revisão não certifica a segurança do aplicativo.

## Correções aplicadas

- `src/Core/Threads/UpdateNames.hpp`: o buffer de 255 bytes era anunciado como tendo 8192 bytes. Agora está inicializado, o tamanho passado é `sizeof(value)` e a chamada usa explicitamente `RegGetValueA`, compatível com o buffer de caracteres. Valores maiores são rejeitados pelo tratamento de erro existente; não há suporte novo a caminhos longos.
- `src/Includes/Utils.hpp`: `GetClipboard()` verifica abertura, handle, tamanho e resultado de `GlobalLock`. A procura pelo terminador fica limitada ao tamanho da memória. Guardas locais liberam o lock e fecham o clipboard também se a construção de `std::string` lançar uma exceção. O formato CF_TEXT foi preservado.

## Demais apontamentos: verificados, sem implementação nesta revisão

1. **Autorização local:** confirmado. `IsAuthenticated()` consulta um estado de página e a interface consulta `IsLogged`. Existe ainda a função `CheatLogin`, que define o estado diretamente; a busca na árvore `src` encontrou somente sua definição. Não foi comprovada uma chamada ativa a ela. Dados criptografados no cliente não garantem resistência contra um executável adulterado. Operações que exijam autorização confiável precisam de validação do lado do servidor.
2. **TLS:** o cliente configura validação de certificado e hostname, exige HTTPS, rejeita redirecionamentos e exige HTTP 200. Não configura pinning nem verificação criptográfica da resposta. A afirmação de exploração por proxy não foi testada. Não foram inventados pins ou chaves do serviço.
3. **HWID:** confirmado uso de SID e retorno literal `none` em falhas. SID identifica a conta, não o hardware. Isso não demonstra, por si só, que contas compartilham uma licença: esse resultado depende da política do servidor. Alterar o identificador demanda compatibilidade e migração das licenças existentes; permanece pendente.
4. **AntiCrack:** o conteúdo está comentado e inclui ações contra processos e serviços. Há também exploração de driver em `src/Bypass/Manipulation/Bypass.cpp`. Não foram reativadas nem aprimoradas rotinas de evasão, antianálise ou exploração de driver. Anti-debug não substitui autorização.
5. **Revalidação:** não foi encontrada revalidação periódica no gerenciador. A verificação de assinatura no login não cobre revogação posterior. Heartbeat permanece pendente.

## Validação

Executado `python tests/crash_regression_checks.py` com MSVC: **9 casos passaram**. O teste extrai os trechos reais modificados e usa mocks das APIs Win32. Cobre clipboard indisponível, ausência de texto, falha de lock, texto vazio, texto válido, falta de terminador e valores de Registro com 255, 256 e 8192 bytes. Verifica também liberação dos recursos nos caminhos exercitados.

Não houve execução do aplicativo, acesso ao KeyAuth, alteração do Registro ou leitura do clipboard real. A compilação completa e os testes de integração continuam pendentes. As alterações anteriores do workspace foram preservadas; nenhum executável foi atualizado.
