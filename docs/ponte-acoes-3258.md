# Ações integradas à build 3258

## Menus
- Personagem → Modelos: modelos de ped/animais, nome personalizado e restauração da aparência original.
- Personagem → Animações: dicionário/clip, repetição e parada.
- Teleportar → Jogadores: Ir ate o jogador, Puxar jogador, Observar jogador e Clonar aparencia, nessa ordem. A aba Lista e acoes foi removida da categoria Jogadores.
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

O protocolo IPC passou para a versao 4. Substitua o EXE e a DLL juntos e reinicie completamente o FiveM. As novas alteracoes ainda precisam de confirmacao em jogo.

Os controles de regeneracao de vida/colete, zerar vida e aplicar vida maxima foram movidos de Extras para Geral, logo abaixo de Restaurar vida e colete, em uma coluna.

Observar jogador tambem usa a camera nativa da ponte: o alvo e localizado pelo ID do servidor, a camera acompanha o ped carregado e a observacao termina ao sair do alcance de carregamento, perder o heartbeat ou selecionar Parar observacao. Copiar roupa foi removido da interface; Clonar aparencia permanece. Ir ate o jogador e Puxar jogador ficam em Teleportar/Jogadores. O usuario confirmou a clonagem e o ajuste de distancia da camera em jogo antes desta atualizacao.

Copiar Discord e Copiar Steam tambem foram removidos da interface.

Chuva de carros e Enviar veiculos ao jogador agora enumeram os veiculos por natives CFX, usam os natives de transformacao/fisica e verificam a posicao 100 ms depois. A operacao e limitada a dez veiculos vazios num raio de 200 m. Entidades de rede sem controle do cliente sao ignoradas; nao ha tentativa de forcar propriedade. A resposta informa confirmados localmente, sem controle, ocupados, fora de alcance e nao confirmados. O iterador e fechado mesmo em caso de falha. Uma confirmacao local nao garante permanencia no servidor. Referencia da enumeracao: https://github.com/citizenfx/fivem/blob/master/code/components/extra-natives-five/src/PoolTraversalNatives.cpp .

Todas as acoes da lista de jogadores ficam em Teleportar/Jogadores. A categoria Jogadores apresenta apenas ESP e Radar e extras. A opcao Parar observacao aparece depois de Clonar aparencia enquanto a observacao estiver ativa.

## Catalogo de modelos disponiveis
Em Carro/Gerar, Carregar lista de modelos consulta GET_ALL_VEHICLE_MODELS, incluindo modelos originais e personalizados registrados no cliente. A busca filtra pelo codigo do modelo; selecionar um item preenche Nome personalizado. A lista nao classifica veiculos VIP e nao concede garagem ou permissoes do servidor. A lista e limitada a 8192 nomes, e e limpa ao reiniciar a sessao; use o botao para atualizar apos mudar de servidor.

O retorno documentado e um fx::scrObject com um array MessagePack de strings. O parser aceita apenas esse formato, com limites de tamanho e quantidade, valida os nomes, ordena e remove duplicatas. O IPC usa protocolo 6 para compartilhar a lista com leitura protegida por contador de sequencia. Referencias: https://github.com/citizenfx/fivem/blob/master/ext/native-decls/GetAllVehicleModels.md e https://github.com/citizenfx/fivem/blob/master/code/components/extra-natives-five/src/ModelListNatives.cpp .


## Gerar veiculos: lista simples com busca
A tela voltou ao formato anterior: Carregar lista de modelos, Buscar veiculo, Escolher veiculo, Nome personalizado, Solicitar veiculo em rede, Entrar no veiculo gerado e Gerar veiculo. Os filtros Tipo, Categoria e Classe foram removidos. A busca ignora maiusculas/minusculas e filtra por parte do nome/codigo do modelo registrado; selecionar um resultado preenche o campo de geracao. O arquivo de categorias nao e mais lido pela interface.

## Reparo completo
Reparar veiculo usa SET_VEHICLE_FIXED, SET_VEHICLE_DEFORMATION_FIXED, saude de motor/carroceria e libera a dirigibilidade. Verifica os oito indices de portas apos o reparo e informa portas ainda danificadas. Atua apenas no veiculo atual; veiculos de rede sem controle local sao recusados, sem forcar posse. Referencia: https://github.com/citizenfx/natives/blob/master/VEHICLE/SetVehicleFixed.md . Protocolo 6: atualizar EXE e DLL juntos e reiniciar completamente FiveM.

## Exclusao de veiculo proximo e modo em rede
Carro/Geral tem Apagar veiculo proximo (15 m), adaptado da HyperX. Enumera veiculos carregados com FIND_FIRST_VEHICLE/FIND_NEXT_VEHICLE, seleciona o mais proximo elegivel em 15 m e recusa veiculo atual, ocupado ou sem controle local; usa SET_ENTITY_AS_MISSION_ENTITY e DELETE_VEHICLE e confirma a ausencia local da entidade. A enumeracao e limitada a 512 veiculos e fecha o iterador mesmo em falha; nao e uma limpeza em massa. Referencias: https://github.com/citizenfx/natives/blob/master/VEHICLE/GetClosestVehicle.md e https://github.com/citizenfx/natives/blob/master/VEHICLE/DeleteVehicle.md .

Gerar em rede fica marcado por padrao. Desmarcar cria um veiculo local visivel apenas para o cliente; isso explica o video enviado pelo usuario, que confirmou ter deixado essa opcao desmarcada. Em rede, a ponte exige NetworkGetEntityIsNetworked, ID de rede positivo e NETWORK_DOES_NETWORK_ID_EXIST antes de confirmar o registro e entrar no veiculo. Aguarda ate 2 segundos pelo registro, sem gerar copias nem fazer fallback para o modo local. O ID compartilhado localmente nao prova recebimento por outro cliente nem concede garagem/permissoes do servidor. Referencia: https://github.com/citizenfx/natives/blob/master/NETWORK/NetworkGetNetworkIdFromEntity.md .


## Correcao do seletor de modelos
O seletor passa a quantidade real de resultados para o BeginCombo personalizado, em vez de zero. Reserva altura de uma a oito linhas e usa a mesma altura nas linhas clicaveis e no clipper; selecionar copia o codigo e fecha a lista. A busca e os modos de geracao permanecem na tela simples.

CREATE_VEHICLE retornando zero em rede informa somente que nao houve uma entidade valida; a causa nao foi confirmada por logs do servidor. OneSync permite recusar criacao pelo cliente via entity lockdown/evento entityCreating. Nesses casos a integracao deve ser feita em um recurso autorizado do servidor, com validacao de permissao e criacao server-side. Nao ha fallback automatico para veiculo local nem contorno das restricoes de servidores de terceiros. Referencia: https://docs.fivem.net/docs/scripting-reference/onesync/ .


## Busca de veiculo para apagar
A exclusao deixa de usar GET_CLOSEST_VEHICLE, que pode omitir tipos de veiculo conforme os flags. A varredura local verifica distancia real ao personagem e escolhe somente um veiculo vazio, dentro de 15 m e sob controle local, ignorando o veiculo atual. Sem candidato, mostra contagens de ocupados, sem controle e fora do raio. Nao solicita posse nem contorna restricoes do servidor. A verificacao de ausencia depois da exclusao permanece.
