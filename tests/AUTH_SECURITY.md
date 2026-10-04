# Reforço da autenticação

Alterações aplicadas à árvore principal `src`, sem modificar backups ou `restored-astra`.

- Campos do formulário são codificados individualmente, inclusive UTF-8 e separadores.
- Resultado assíncrono publicado por flag atômica; a interface aguarda o término da thread antes de consumir o resultado. Uma nova tentativa não sobrescreve resultado pendente.
- Inicialização rejeita sessão vazia e limpa sessão anterior antes de nova inicialização.
- Login exige usuário, lista de assinaturas e ao menos uma assinatura com nome e prazo futuro válido. Campos ausentes, números inválidos, overflow, prazo zero e prazo vencido não autorizam acesso.
- Cadastro bem-sucedido passa pelo login e pela mesma validação de assinatura.
- Transporte exige HTTPS, valida certificado/hostname, não segue redirecionamentos, exige HTTP 200 e limita tamanho e duração da resposta.

Compatibilidade: o prazo `expiry` é esperado como string decimal Unix, conforme o formato já usado pelo cliente. Assinaturas vitalícias precisam trazer um prazo futuro explícito; zero não é aceito como autorização implícita. A política aceita qualquer assinatura ativa da aplicação, como antes; não introduz distinção de produtos.

Validação: `auth_security_checks.cpp` compilado com MSVC v143 x64 e executado com `Failures: 0`. Cobre caracteres especiais, UTF-8, campos ausentes, licença válida/vencida, prazo inválido/overflow, múltiplas assinaturas e consumo prematuro do resultado assíncrono. O gerenciador completo é incluído no teste. Houve aviso preexistente C4244 na conversão do SID.

O MSBuild encontrou falha de acesso no FileTracker deste ambiente; a compilação e execução do teste foram feitas diretamente com o compilador MSVC. O aplicativo completo não foi recompilado nem executado. Não houve chamadas ao serviço KeyAuth; cadastro/login reais e a convenção de licenças vitalícias precisam ser verificados antes da distribuição.

Limites: são correções de autenticação, não proteção contra alteração arbitrária do executável. Não foram implementados antidebug, ocultação, proteção de driver, assinatura criptográfica das respostas nem revalidação periódica de sessão no servidor. Revogação após login não está coberta por estas mudanças. Nenhum binário anterior foi atualizado.
