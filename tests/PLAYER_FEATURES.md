# ESP Name e God Mode — verificacao de 2026-10-05

Na build b3258 em execucao, o ID em `CPlayerInfo + 0xE8` coincidiu com
os IDs da lista de nomes. `CPed + 0xE8` continha outros dados e
`CPlayerInfo + 0x7C` nao fornecia o ID. O dump `offsets.h` nao deve ser
tratado como validacao semantica desses campos.

A lista observada tinha 697 entradas na primeira leitura, acima do limite
antigo de 256. A implementacao nova valida a lista circular do MSVC,
le os nomes curtos ou alocados separadamente e usa IDs exatos. O endereco
da lista e descoberto no modulo carregado, sem fixar seu RVA.

O God Mode agora verifica leitura, escrita e retorno dos flags. A opcao
salva e reaplicada ao jogador atual; teleporte restaura a opcao do usuario.
Os bits nao relacionados sao preservados. A interface informa falha de
aplicacao, em vez de sempre anunciar sucesso.

## Validacao

Execute `python tests/verify_player_features.py` para compilar e executar
os testes locais. Eles usam a implementacao de producao, uma lista MSVC
real com 697 nomes, um nome UTF-8 longo, uma lista corrompida e memoria
do proprio teste para verificar IDs e flags. Nao alteram o jogo.

Opcionalmente, `python tests/verify_player_features.py --live PID` testa
somente a leitura de nomes em um FiveM b3258 aberto. Pode exigir executar
o terminal como administrador para enumerar os modulos.

Resultado observado com a nova funcao: **39/39 nomes resolvidos**, incluindo
5 nomes longos. Compilacao Release x64 concluida, com os avisos anteriores
C4244 e LNK4098. Nao houve teste de dano ou de renderizacao na partida.

## Referencias

- [Implementacao de nomes do CitizenFX](https://github.com/citizenfx/fivem/blob/master/code/components/citizen-playernames-five/src/HookPlayerNameHandling.cpp)
- [Flags de invencibilidade documentados pelo CitizenFX](https://github.com/citizenfx/natives/blob/master/ENTITY/SetEntityInvincible.md)
