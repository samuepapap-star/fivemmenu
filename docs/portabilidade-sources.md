# Integração das sources restantes

Base: feature/shark-integration, commit 77ad0cb. Esta alteração NÃO conclui todos os itens do levantamento.

## Implementado
- Distância automática do SilentAim: limita a distância manual pelo alcance válido da arma, como na Traceless; falha de leitura preserva o limite manual. Campo de alcance habilitado somente nas builds cujo layout já era aceito pelo projeto (2060–2802, nas versões explicitamente enumeradas).
- Distância da câmera: controle independente de FOV, com a transformação da Renaissance/Priv XRC; restaura o campo do modelo ao desligar, morrer, entrar no carro ou encerrar normalmente. Somente valores entre -5 e 1 são aceitos como originais.
- As duas opções participam da configuração exportada/importada.
- Diagnóstico de dependências: padrões de registro de natives e relógio da Renaissance adicionados ao coletor somente de leitura; contagens de correspondências e valores do relógio servem para avaliar compatibilidade, sem habilitar essas funções.

## Horário local integrado depois da coleta 3258
Configurações → Ambiente permite aplicar e manter hora/minuto ou usar presets. A revisão do binário, assinatura e campos são verificados antes de escrever. Ao desligar, as escritas cessam e o relógio volta a avançar; o servidor pode ressincronizar. Consulte docs/horario-local-3258.md.

## Diagnóstico necessário
Com o FiveM aberto, execute o novo EXE com o argumento:
```powershell
& '.\Wexize Revamp.exe' --diagnostico-recursos
```
O coletor informa o caminho de ALTOS_PODERES_<build>.json na pasta temporária. Não escreve na memória do jogo, não instala hooks e não inicia o menu. Correspondências de padrões são candidatos, não endereços automaticamente confiáveis.

## Ainda pendente
Ponte/runtime Lua, clima/densidade, tuning cosmético completo e acessórios, spawn/delete, controle remoto, transformações/animais, escala de outros peds, modos extras de freecam, Anti Attack/Kill Aim, Combat Glitch/liberar ações, Solo Session/Fake Lag/Desync, Indicate Headshot e variantes nativas originais de Double Shot/Freeze Ammo 2.

A branch não tem um despachante de natives compatível validado. Não foram copiados os caminhos que liberam memória remota após um timeout sem confirmar o término da thread. Não foi inferido que perda de vida significa headshot: o indicador precisa de um evento/dado de impacto confiável.

O diagnóstico é um pré-requisito para continuar a identificação das dependências; não garante sozinho que todos os natives ou o runtime Lua possam ser integrados.

## Validação
Testes de limites de distância, leitura inválida, NaN/infinito, cap manual e transformação da câmera são executados pelo workflow, seguido da compilação Release x64. Execução no FiveM e efeito/sincronização de cada opção ainda precisam de teste real.
