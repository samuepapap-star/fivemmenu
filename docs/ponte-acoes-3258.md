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
O código recebido em FiveM.zip foi usado para identificar o protocolo de eventos e o ABI. O caminho de execução foi refeito: resolução por exports públicos, confirmação da revisão instalada e despacho de comandos tipados no tick do runtime. Não há reinício de resources, alteração de proteções CitizenFX, nomes de serviços disfarçados, execução arbitrária de Lua ou eliminação de rastros.

A DLL só aceita os componentes identificados nesta máquina: citizen-resources-core.dll (timestamp 1793965266, imagem 782336) e rage-scripting-five.dll (timestamp 1793798977, imagem 1720320). Usa GetCurrent e GetNativeHandler pelos nomes exportados; não usa o antigo endereço fixo 0x257B0.

O jogo informa o carregamento de modelos/assets antes de agir. As respostas distinguem pedido pendente, falha e confirmação. Pedidos têm prazo e cancelamento; perda do heartbeat interrompe o RPG. A troca de modelo é recusada dentro de veículos ou enquanto o ped está vinculado. O modelo/aparência originais são guardados em um clone local invisível; se o personagem mudar fora da operação, o backup é descartado.

## Validação
Testes simulam carregamento atrasado, timeout/cancelamento, alvo que saiu, troca recusada, restauração, criação de veículo, confirmação de animação e expiração do heartbeat do RPG. O workflow compila a DLL e o EXE Release x64. O teste real destas novas ações no FiveM ainda depende do usuário.

## Limites
A ponte não é um executor Lua genérico. A aceitação de modelo, animação e entidades em rede depende do jogo/servidor. O clone usa os natives de modelo e ClonePedToTarget; scripts de aparência do servidor podem reaplicar outro visual. Não é anunciada compatibilidade com outra revisão dos componentes.

Referências de interface: https://github.com/citizenfx/fivem/blob/master/code/components/citizen-resources-core/include/ResourceManager.h e https://github.com/citizenfx/fivem/blob/master/code/components/rage-scripting-five/include/scrEngine.h .
