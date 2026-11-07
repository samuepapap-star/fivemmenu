# Horário local e investigação de natives

## Horário local
Configurações → Ambiente permite escolher hora/minuto, aplicar uma vez, usar presets ou manter o horário. Ao desativar a manutenção, as escritas cessam e o relógio do jogo pode avançar ou ser ressincronizado pelo servidor. Nenhuma instrução do relógio é modificada.

O recurso aceita somente a revisão identificada no relatório do cliente:
- build: 3258;
- PE timestamp: 1793965266;
- tamanho da imagem: 73740288;
- assinatura no RVA 0x600c11 com deslocamento confirmado para o relógio no RVA 0x1dbc220.

Antes de cada aplicação, são conferidos PE, fingerprint, assinatura, destino e intervalos dos campos. Outras revisões são recusadas. Transferências parciais tentam recuperar os campos anteriores somente quando ainda pertencem à própria escrita; alterações concorrentes do jogo são preservadas. Isso confirma apenas o estado local, sem garantir sincronização com o servidor.

## Diagnóstico de natives
O relatório anterior continha 39 correspondências de um padrão genérico. Isso não identificava 39 natives nem um resolver confiável. O esquema 3 registra, por correspondência, o destino da chamada rel32, 512 bytes da rotina e, quando legível, 128 bytes do primeiro nó apontado pela tabela. O coletor permanece somente de leitura e não chama os candidatos.

O título das janelas agora usa a build detectada, eliminando o texto fixo 3889.

## Validação e pendências
O workflow executa os testes de perfil, assinatura/RIP, intervalos, leitura inválida, escrita parcial/rollback, alteração concorrente e resolução de chamadas com deslocamento negativo/fora da imagem; em seguida compila Release x64.

Ainda não há um invoker de natives validado nem uma ponte Lua nesta entrega. Clima, spawn/delete, tuning cosmético completo, transformações e os demais grupos dependentes continuam pendentes. O relógio foi implementado pela estrutura identificada no diagnóstico, sem depender desse invoker. Não houve teste de escrita ao vivo no FiveM neste ambiente.
