# Integração das funções adicionais

Esta entrega integra o conjunto abaixo e organiza os controles por categoria. Não representa 92 funções independentes: o levantamento original contém duplicatas, variantes e grupos de várias ações.

## Implementado nesta entrega

| Menu | Controles e ações |
|---|---|
| Personagem → Geral | Funções anteriores, incluindo restauração de vida e colete em um clique. |
| Personagem → Extras | Regeneração de vida enquanto vivo, regeneração de colete, zerar vida, ajustar vida máxima, super pulo, pulo da besta, bloqueio local de lock-on, Spinbot, câmera livre independente e velocidade da câmera. |
| Carro → Extras | Destrancar o mais próximo, destrancar/trancar próximos, boost por tecla, Rocket Boost, carro pulando/planando, tração máxima, força/altura da suspensão, gravidade, drift, preset de aderência e restauração dos ajustes adicionais. |
| Carro → Lista | Puxar da XRC Updated e ações anteriores. O agarre continua ativo ao interagir com o próprio overlay e movimenta com o menu aberto. O arremesso anterior não foi declarado validado. |
| Carro → ESP | Marcador de veículos e ignorar ocupados, além dos controles anteriores. |
| Carro → Enviar ao jogador | Enviar veículos e a variante chuva de carros sobre a posição do alvo. Confirma escritas locais, não sincronização do servidor. |
| Jogadores → Lista e ações | Observar/parar observação e puxar jogador, além das ações anteriores. |
| Jogadores → Radar e extras | Radar com jogadores, NPCs, veículos, nomes, rotação com câmera, alcance, tamanho e posição arrastável; tamanho/fonte de texto ESP; espessura de caixas/esqueleto; China Hat; ícones de armas do catálogo existente; aviso heurístico de invisíveis; hotkey de ESP. |
| Armas → Extras | Congelar quantidade de munição, punho explosivo, munição incendiária/explosiva, velocidade de recarga, multiplicador do intervalo entre disparos e escala da arma. Campos só são usados após leituras válidas e testes de faixa. |
| Mira silenciosa | Cabeça/pescoço/tronco; predição com antecipação ajustável; filtro somente motoristas; FOV dinâmico com mínimo. |
| Tiro automático | Seleção cabeça/pescoço/tronco no modo de seleção por FOV; o modo inteligente continua usando o alvo apontado pelo jogo. |
| Teleportar → Posição salva | Salvar posição atual, retornar, hotkeys de salvar/retornar e coordenadas. A posição é descartada ao trocar de processo/personagem. |
| Configurações → Aparência | Cores animadas, estilos atual/arredondado/quadrado, fundos Matrix/estrelas/ondas/corações/vórtice/raios/galáxia/chuva, borda, partículas com quantidade/velocidade/tamanho, ocultar da barra de tarefas, projeção do overlay para outro monitor, retículo com tipo/tamanho/espessura/cor. |
| Configurações nomeadas | Novos ajustes incluídos na exportação/importação, com verificação de tipos e limites. |

Câmera livre e observação usam a mesma rotina verificada do jogo: se não houver uma assinatura única com os deslocamentos esperados, a ativação é recusada com uma mensagem. Não há tentativa de adivinhar outro endereço. A projeção em outro monitor move o overlay mantendo a resolução do jogo; não é hardware DMA nem captura/transmissão de vídeo.

## Restauração e leitura de dados

- Leitura das listas por snapshots protegidos pelos mesmos mutexes usados pelos produtores.
- Restauração dos valores de arma/handling e dos bits controlados ao desativar ou encerrar.
- Bits que pertencem a outras funções são preservados; mudança de processo descarta endereços antigos.
- Encerrar restaura a câmera primeiro. Se a restauração da rotina de câmera falhar, a interface informa o erro e não confirma um encerramento limpo.
- A restauração por fechamento normal também é registrada como limpeza do processo.

## Itens fora desta entrega

Foram mantidas as exclusões aceitas: reviver completo controlado pelo servidor, cargo real de ADM, runtime/bridge Lua, funções que dependem de natives ainda não integrados (tuning cosmético completo, acessórios, spawn/delete, transformações e animais), arquivos vazios/comentados, infraestrutura Cloud, rotinas de crash e contorno de proteções.

Também não há implementação completa dos seguintes grupos do levantamento: hora/clima/otimização de densidade; controle remoto de veículos; distância de câmera; escala arbitrária de outros peds; Anti Attack/Kill Aim; Combat Glitch/liberar ações; sessão solo/fake lag/desync; modos de interação da freecam que dependem de chamadas internas; Automatic Distance/Indicate Headshot; estilos exclusivos que exigem assets externos. Esses itens exigem identificação e validação de dependências específicas e não foram apresentados como botões funcionais. Esta é uma limitação real da integração atual, não uma confirmação de incompatibilidade definitiva.

Rapid Fire foi adaptado como ajuste do intervalo entre disparos, sem importar o hook original da SHARK. Double Shot não foi importado como escrita concorrente no mesmo campo. Freeze Ammo 2 por native invoker não foi integrado; o congelamento de quantidade usa a cadeia de munição externa. Godmode 2.0 não foi anunciado como nova proteção: God, regeneração de vida e regeneração de colete permanecem controles separados.

## Validação

- Testes simulados com ASan/UBSan: restauração de valores, preservação de alterações de outro módulo, descarte ao mudar de processo, aquisição/liberação de bits, configuração com tipos inválidos e limites, reconhecimento e recusa da assinatura de câmera.
- Regressão de veículos: puxar XRC, rollback de escrita, invalidação de entidade e manutenção do agarre ao receber foco no próprio overlay.
- Verificação sintática das novas páginas contra os headers ImGui do projeto.
- Compilação e upload Release x64 precisam constar como sucesso no workflow correspondente ao commit entregue.
- Não houve execução do EXE no FiveM neste ambiente. Não há confirmação de sincronização com servidores, eficácia de God, arremesso, clima ou proteção contra detecção.
