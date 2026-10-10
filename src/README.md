# Estrutura da pasta

Abra `src.slnx` no Visual Studio para acessar o projeto `src.vcxproj`.
O arquivo `src.vcxproj.filters` agrupa os itens do projeto por pasta no
Gerenciador de Soluções. Se necessário, desative **Mostrar Todos os Arquivos**
para visualizar esses grupos.

| Caminho | Conteúdo |
| --- | --- |
| `main.cpp` | Ponto de entrada do aplicativo |
| `Globals.hpp` | Definições globais |
| `Auth/` | Arquivos de autenticação |
| `Core/` | Código principal, SDK, recursos e threads |
| `Gui/` | Interface, páginas, overlay e imagens |
| `Includes/` | Cabeçalhos auxiliares e bibliotecas incorporadas |
| `Security/`, `Bypass/` | Módulos existentes do projeto |
| `packages/`, `packages.config` | Dependências NuGet |
| `x64/` | Saída atual de compilação, mantida no caminho original |
| `build/archive/` | Artefatos de compilações anteriores |
| `.vs/`, `src.vcxproj.user` | Configurações locais do Visual Studio |

## Organização realizada

- `svchost/` foi movida para `build/archive/svchost/`.
- `Gosth Fivem/` foi movida para `build/archive/Gosth Fivem/`.
- Os 97 arquivos movidos tiveram seu conteúdo verificado por SHA-256.
- `src.vcxproj.filters` foi criado com os mesmos itens declarados no projeto.
- `src.filters`, que já existia, foi preservado.

Os arquivos de código, as dependências e as configurações de compilação
foram mantidos. Nenhum arquivo foi excluído. Para desfazer o arquivamento,
mova as duas pastas de `build/archive/` de volta para esta pasta raiz.

A organização não inclui uma compilação do projeto nem a correção de
referências preexistentes a arquivos ausentes.
