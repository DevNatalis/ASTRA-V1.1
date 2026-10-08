# ASTRA — canal de atualização (launcher + auto-update)

Fluxo: `ASTRA.exe` → login (KeyAuth, inalterado) → **launcher**
(abas Inject / Descrição / Update) → botão **Inject** → painel principal.

## 1. Arquivos do canal

| Arquivo | Papel |
|---|---|
| `src/Core/Update/UpdateConfig.hpp` | Fonte única de versão (`1.0.0`), endpoint do manifesto (`DevNatalis/ASTRA-V1.1` → `releases/latest/download/manifest.json`), Discord, status padrão, chave pública RSA **de produção** (fingerprint do módulo SHA-256: `7a7af9f1…9688c73a`; privada fora do repo + secret `UPDATE_SIGNING_KEY`). |
| `src/Core/Update/UpdateCrypto.hpp` | SHA-256 + RSA PKCS#1 v1.5/SHA-256 via CNG (BCrypt). Compartilhado com o Updater. |
| `src/Core/Update/UpdateManifest.hpp` | SemVer estrito, validação de URL HTTPS, parse/validação do `manifest.json`, payload canônico de assinatura. |
| `src/Core/Update/UpdateManager.hpp` | Worker assíncrono (nunca bloqueia a UI): check, download com progresso, verificação, disparo do Updater, trava de update obrigatório. |
| `src/Gui/Pages/Launcher.hpp` | UI das 3 abas no UI kit existente (sidebar `NavItem`, partículas, `PrimaryButton`, `BeginCard`). |
| `src/Updater/Updater.cpp` + `UpdaterCore.hpp` + `Updater.vcxproj` | Executável separado: espera o app fechar, re-verifica hash+assinatura, backup `.bak`, troca (cross-volume via `COPY_ALLOWED`), rollback em falha, reinicia. |
| `src/tests/update_regression.{cpp,cmd}` | Testes: semver, URLs, parse de manifesto, vetor SHA-256, round-trip RSA via BCrypt. |
| `src/tests/update_e2e.cpp` + `run_update_e2e.ps1` + `tls_server.ps1` | E2E isolado (nunca toca a instalação real): servidor TLS localhost, fixtures assinadas com a chave de produção, `UpdateManager` real, `Updater.exe` real (swap + relaunch), rollback passo a passo; matriz: happy path, download interrompido, manifesto adulterado, assinatura forjada, versão antiga, falha de troca, obrigatório. |
| `.github/workflows/build-release.yml` | Build Release x64 (ASTRA + Updater), testes, Release assinada em tag `vX.Y.Z`. |
| `scripts/new_update_keys.ps1` | Gera o par RSA-2048 (máquina do mantenedor, offline). |
| `scripts/sign_manifest.ps1` | Gera `manifest.json` assinado de um build local. |

`Gui.cpp` apenas desvia `IsLogged && !injected` para o launcher e libera o
painel no Inject (com `RequireAuth()` + trava de obrigatório). Nada do
painel ou do auth foi alterado.

## 2. Configuração inicial (uma vez)

1. Gere as chaves (máquina segura, **nunca** commite a privada):
   `powershell -ExecutionPolicy Bypass -File scripts/new_update_keys.ps1`
2. Cole o snippet `public_key_snippet.txt` em `UpdateConfig.hpp`
   (`UpdatePublicKey::Exponent/Modulus`), substituindo o placeholder zerado.
   Sem isso, **toda** assinatura falha fechado (fail closed) — por design.
3. Ajuste em `UpdateConfig.hpp`: `kAppVersion` (`"1.0.0"`),
   `ManifestUrl()` (`https://github.com/<owner>/<repo>/releases/latest/download/manifest.json`),
   `DiscordUrl()`.
4. No GitHub: Settings → Secrets → Actions → `UPDATE_SIGNING_KEY` = conteúdo
   de `scripts/update_keys/update_priv.pem` (o workflow nunca loga esse valor).
5. Garanta que `Updater.exe` é distribuído **ao lado** do `ASTRA.exe`
   (o instalador procura no próprio diretório).

## 3. Publicando a primeira atualização

1. Suba a versão em `UpdateConfig.hpp` (`kAppVersion = "1.1.0"`), commit.
2. Crie e envie a tag: `git tag v1.1.0 && git push origin v1.1.0`.
3. O workflow compila, roda os 3 testes, verifica `tag == kAppVersion`,
   assina o manifesto com o secret e publica a Release com
   `ASTRA.exe` + `Updater.exe` + `manifest.json`.
4. Teste local antes: rode `scripts/sign_manifest.ps1` apontando `-Url`
   para o asset real e valide contra o cliente.

Publicação manual (sem CI): compile Release x64, rode `sign_manifest.ps1`,
suba os 3 arquivos para o hosting HTTPS e aponte `kManifestUrl`.

## 4. Modelo de segurança (resumo)

- Confiança só na assinatura RSA embarcada (hash do mesmo servidor sozinho
  nunca basta); payload canônico `version\nsha256\nurl`.
- SemVer estrito rejeita versões inválidas (sem downgrade silencioso:
  manifesto mais velho que o instalado resulta em "atualizado").
- HTTPS com `VERIFYPEER/HOST`, só esquema `https`, redirects só HTTPS.
- Nome de staging fixo em `%TEMP%` (nunca usa o path remoto — sem traversal;
  nunca baixa em diretório sensível); tampões de tamanho (64KB/200MB).
- Executável instalado só é tocado pelo Updater **após** re-verificação, com
  `.bak` + rollback; falha em qualquer etapa mantém a versão anterior.
- `mandatory:true` (ou instalado < `min_version`) bloqueia o Inject.
- Downloads em threads worker; UI faz poll por frame (sem freeze).

## 5. Dependências externas de runtime

- Servidor KeyAuth (auth existente, inalterado).
- Endpoint HTTPS do `manifest.json` + asset do `.exe` (GitHub Releases serve).
- Segredo `UPDATE_SIGNING_KEY` no GitHub para releases assinadas pelo CI.
- `Updater.exe` ao lado de `ASTRA.exe` (gerado pelo mesmo build/Release).
