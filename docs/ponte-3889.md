# Ponte 3258 / 3889 — pacote para validacao no jogo

O EXE e FiveMMenuBridge.dll continuam juntos; este caminho nao instala resource no servidor.
Use apenas onde as acoes sao autorizadas. Restricoes de rede, servidor e carregamento nao sao contornadas.

## Compatibilidade e verificacoes

- Protocolo 7: reinicie o FiveM e extraia EXE e DLL do mesmo artefato. Uma DLL antiga e recusada.
- A ponte aceita os executaveis 3258 e 3889 e confirma GET_GAME_BUILD_NUMBER na thread de script antes de executar acoes.
- A 3889 conserva exatamente os dois perfis PE ja registrados em Build3889Compatibility: tamanho 65387520, timestamps 1793798832 e 1793965275. Revisoes diferentes continuam bloqueadas.
- Os tres perfis CFX, exports, contexto de script, verificacoes de memoria e tratamento de falhas permanecem.
- Os offsets existentes da 3258 nao foram alterados. Os offsets de leitura da 3889 nao foram ampliados.
- m_BlipList da 3889 permanece zero: nao existe evidencia de um endereco valido. O botao de waypoint das builds 3258 e 3889 usa natives de marcador, carregamento de terreno e posicionamento, sem acessar essa lista.

## Teleporte e reparo

O waypoint exige personagem vivo, desvinculado, a pe ou motorista sem passageiros e controle local.
Cancela se marcador, personagem, veiculo ou controle mudar. Carrega o destino por ate cinco segundos,
recusa outra cena de carregamento ativa, exige terreno valido e move uma unica vez.
Sem terreno carregado, nenhuma posicao ficticia (como Z=-210) e usada.
A confirmacao local ocorre apos 250 ms, com tolerancia de cinco metros; nao garante permanencia nem sincronizacao no servidor.
Uma correcao do servidor nunca causa nova tentativa automatica.

O reparo preserva a recusa de veiculos sem controle local. Confirma motor, carroceria e portas,
em vez de considerar apenas as portas como prova de sucesso. A verificacao e local, nao uma garantia de sincronizacao.

## Testes automatizados e limites

GitHub Actions executa testes C++ de politica de build, waypoint, reparo e a suite preexistente,
depois compila DLL e EXE. Os testes usam natives simulados: nao comprovam ABI real, streaming real,
comportamento de todos os veiculos, funcionamento de todas as funcoes antigas ou autorizacao de qualquer servidor.
Os testes de politica aceitam 3258/3889, recusam divergencias e revisoes PE desconhecidas.
Os testes de waypoint abrangem sucesso, terreno ausente/invalido, cancelamento, sessao,
mudanca de alvo, falta/perda de controle, passageiros, falhas de native e ausencia de confirmacao.
As verificacoes de build sao barreiras de compatibilidade, nao permissao do servidor.

## Teste manual pendente

1. Reinicie o FiveM, extraia EXE e DLL juntos e confira commit/hashes em build-info.json.
2. Em ambiente autorizado, confira a build em Configuracoes e o status da ponte apos uma acao.
3. Teste reparo num veiculo danificado que voce controla, inclusive portas; confira resultado visual e status.
4. Teste waypoint proximo e distante, a pe e dirigindo sozinho, e ausencia/remocao do marcador.
5. Confira que falta de controle, passageiro, terreno indisponivel e troca de sessao cancelam sem repetir.
6. Repita na 3258 para verificar regressao. Registre build, revisao CFX, commit e mensagem exata em qualquer falha.

O pacote e destinado a teste. Nenhuma execucao no jogo foi realizada durante a implementacao.
As demais funcoes da ponte usam o caminho existente; nao foram certificadas individualmente na 3889.

Referencias oficiais:
- https://docs.fivem.net/docs/server-manual/server-commands/#sv_enforcegamebuild-build
- https://github.com/citizenfx/natives/blob/master/MISC/GetGroundZFor_3dCoord.md
- https://github.com/citizenfx/natives/blob/master/STREAMING/NewLoadSceneStartSphere.md
- https://github.com/citizenfx/natives/blob/master/ENTITY/SetEntityCoordsNoOffset.md
