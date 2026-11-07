# Ações integradas à build 3258

## Menus
- Personagem → Modelos: modelos de ped/animais, nome personalizado e restauração da aparência original.
- Personagem → Animações: dicionário/clip, repetição e parada.
- Jogadores → Lista e ações: Clonar aparência do jogador selecionado; o comando antigo Copiar roupa permanece separado.
- Carro → Gerar: modelo, entrada no veículo e solicitação opcional em rede.
- Mira silenciosa: Silent RPG sem equipar a arma, disparado no alvo válido dentro do FOV com o botão esquerdo; desativa a mira silenciosa padrão enquanto ativo.

## Instalação
Extraia o EXE e FiveMMenuBridge.dll na mesma pasta. O menu só carrega a ponte ao solicitar uma dessas ações; a DLL é compilada neste repositório. A ponte fica carregada no FiveM até o encerramento do processo porque o jogo passa a possuir callbacks dela.

## Execução
O código recebido em FiveM.zip foi usado para identificar o protocolo de eventos e o ABI. O caminho de execução foi refeito: resolução por exports públicos, confirmação da revisão instalada e despacho de comandos tipados em uma CfxThread propria. O tick do ResourceManager apenas inicializa e registra essa thread; os natives rodam em DoRun, com o contexto de script atribuido pelo proprio runtime. A ponte confere que a thread ativa corresponde a sua thread antes de cada chamada. Não há reinício de resources, alteração de proteções CitizenFX, nomes de serviços disfarçados, execução arbitrária de Lua ou eliminação de rastros.

A DLL só aceita os componentes identificados nesta máquina: citizen-resources-core.dll (timestamp 1793965266, imagem 782336) rage-scripting-five.dll (timestamp 1793798977, imagem 1720320) e scripting-gta.dll (timestamp 1793799053, imagem 286720). Usa GetCurrent e GetNativeHandler pelos nomes exportados; não usa o antigo endereço fixo 0x257B0.

O jogo informa o carregamento de modelos/assets antes de agir. As respostas distinguem pedido pendente, native ausente, excecao durante a execucao (com hash e codigo SEH) e confirmacao. Pedidos têm prazo e cancelamento; perda do heartbeat interrompe o RPG. A troca de modelo é recusada dentro de veículos ou enquanto o ped está vinculado. O modelo/aparência originais são guardados em um clone local invisível; se o personagem mudar fora da operação, o backup é descartado.

## Validação
Testes simulam carregamento atrasado, timeout/cancelamento, alvo que saiu, troca recusada, restauração, criação de veículo, confirmação de animação e expiração do heartbeat do RPG. O workflow compila a DLL e o EXE Release x64. O teste real destas novas ações no FiveM ainda depende do usuário.

## Limites
A ponte não é um executor Lua genérico. A aceitação de modelo, animação e entidades em rede depende do jogo/servidor. O clone usa os natives de modelo e ClonePedToTarget; scripts de aparência do servidor podem reaplicar outro visual. Não é anunciada compatibilidade com outra revisão dos componentes.

Referências de interface: https://github.com/citizenfx/fivem/blob/master/code/components/citizen-resources-core/include/ResourceManager.h e https://github.com/citizenfx/fivem/blob/master/code/components/rage-scripting-five/include/scrEngine.h .

A interface CfxThread importada e documentada em https://github.com/citizenfx/fivem/blob/master/code/components/rage-scripting-five/include/scrThread.h ; sua implementacao em https://github.com/citizenfx/fivem/blob/master/code/components/rage-scripting-five/src/scrThread.cpp configura a thread ativa durante DoRun. A biblioteca de importacao e gerada a partir dos nomes exportados, sem enderecos internos fixos.

Quando o modelo base do alvo ja e o mesmo do personagem, a clonagem copia os componentes e aderecos diretamente, preservando o ped. A restauracao no mesmo modelo tambem evita REQUEST_MODEL e SET_PLAYER_MODEL; modelos diferentes continuam pelo carregamento assincrono na thread propria.

## Atualizacao de interface, veiculos e camera livre
O menu aberto recebe foco e usa as mensagens normais do teclado; o encaminhamento global de teclas foi removido. Ha seletores por mouse para veiculos e animacoes, mantendo os campos personalizados editaveis.

A geracao de veiculo informa modelo e modo quando CREATE_VEHICLE retorna zero, solicita uma entidade vinculada ao script e aguarda ate dois segundos se um handle ainda nao estiver visivel. Nao ha repeticao automatica nem troca silenciosa do modo em rede para local. O servidor pode recusar entidades criadas pelo cliente, conforme https://docs.fivem.net/docs/scripting-reference/onesync/ .

A camera livre em Personagem/Extras agora usa uma camera propria, por natives, na CfxThread da ponte. Nao depende da assinatura de codigo da camera antiga. Feche o menu para mover: WASD, mouse, Espaco/Ctrl; Shift dobra a velocidade; Esc encerra. O slider em m/s controla apenas o deslocamento da camera livre. A distancia da camera existente permanece independente. A camera criada e destruida pela ponte, e o foco de streaming e restaurado ao parar ou perder o heartbeat. NoClip e sem colisao devem estar desativados.

O protocolo IPC passou para a versao 3. Substitua o EXE e a DLL juntos e reinicie completamente o FiveM. As novas alteracoes ainda precisam de confirmacao em jogo.

Os controles de regeneracao de vida/colete, zerar vida e aplicar vida maxima foram movidos de Extras para Geral, logo abaixo de Restaurar vida e colete, em uma coluna.
