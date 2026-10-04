# ASTRA

Projeto C++ para Windows / Visual Studio. Esta copia contem a arvore principal `src`, os testes de autenticacao e o utilitario `fivem-offset-dumper-main`.

## Compilacao

Abra `src/src.slnx` em uma versao compativel do Visual Studio, com ferramentas C++ e Windows SDK instalados. Restaure os pacotes NuGet declarados em `src/packages.config`. Revise os caminhos locais do DirectX SDK em `src/src.vcxproj` conforme seu ambiente.

As bibliotecas estaticas locais libcurl e FreeType foram preservadas por serem dependencias do projeto. A compilacao completa desta exportacao ainda nao foi validada.

## Autenticacao

Veja `tests/AUTH_SECURITY.md` para as mudancas, testes e limitacoes. A configuracao da aplicacao KeyAuth permanece em `src/Auth/auth_manager.hpp`. Nao adicione senhas, tokens GitHub ou chaves administrativas ao repositorio.

## Conteudo excluido

Backups, recuperacoes, variantes antigas, caches do Visual Studio, pacotes NuGet restauraveis, executaveis e saidas de compilacao nao fazem parte desta copia. Os arquivos originais no workspace foram preservados.
