# SkinChangerLocal — integração autorizada

O EXE conserva os índices globais 0–27 e as categorias existentes. Os botões
**Copiar comando Aplicar/Restaurar** apenas colocam um comando na área de
transferência. Cole no chat do FiveM (ou retire a barra inicial no F8).
Nenhuma mensagem no EXE afirma que copiar equivale a aplicar.

O administrador instala este recurso em seu servidor. `RegisterCommand`
restringe `skinlocal` à permissão ACE `command.skinlocal`; o servidor aceita
somente um índice e encaminha exclusivamente seu próprio mapeamento aprovado.
O cliente recusa eventos locais e verifica a arma e o resultado das nativas.
A confirmação aparece no chat e no F8, não no EXE. Ela é uma leitura do cliente
naquele momento, não uma garantia de persistência nem uma prova independente
de um cliente não modificado.

## Instalação

1. Copie esta pasta para `resources/SkinChangerLocal`.
2. Configure `SkinChangerConfig.approved` em `config.lua` usando os metadados
   reais dos componentes instalados. A tabela vem vazia: as 28 entradas do
   catálogo histórico não são componentes confirmados nem autorizações.
3. Registre os `data_file` reais no manifesto caso use componentes customizados.
4. No `server.cfg`, conceda acesso somente ao grupo desejado:
   ```cfg
   add_ace group.admin command.skinlocal allow
   ensure SkinChangerLocal
   ```
   A associação dos administradores ao grupo deve existir no servidor.
5. Use `/skinlocal apply 0` ou `/skinlocal restore`.

## Formato dos mapeamentos

Exemplo de teste **de tint**, não equivalente a uma skin Angel Love:
```lua
SkinChangerConfig.approved[0] = {
    enabled = true, verified = true,
    mode = 'tint', baseWeapon = 'WEAPON_PISTOL', tintIndex = 1
}
```
Coloque a atribuição ao final de config.lua. O administrador decide se
essa associação é apropriada; o exemplo não vem habilitado.

Para componentes, use `mode = 'component'`, `baseWeapon`,
`componentHash`, `enabled = true`, `verified = true`,
`componentGroupVerified = true` e `componentGroup = { ... }`.
O grupo deve conter o componente escolhido e **todos os componentes mutuamente
exclusivos daquele encaixe**, conforme os metadados do pack. Não declare
o grupo verificado sem validar sua completude. O recurso captura esse grupo
antes da aplicação e o restaura, incluindo componentes que já existiam.
Não há enumeração automática de todos os componentes customizados de uma arma.

O novo catálogo proposto na conversa usa nomes candidatos e redistribui IDs
já usados. Não foi usado para substituir silenciosamente o catálogo existente.
Os modelos add-on históricos continuam listados, mas aplicação add-on não é
implementada: trocar armas exige integração específica com o inventário do
servidor. O recurso não cria armas, munição, nem remove a arma original.

## Restauração e limites

A restauração usa personagem e arma capturados, mesmo após trocar a arma
equipada. Se a arma foi retirada ou o personagem mudou, não a recria. Se o tint
ou grupo mudou externamente, preserva essa mudança e descarta o estado antigo.
Falhas das nativas não geram confirmação de sucesso; o estado de restauração
é retido quando possível. Parar o recurso tenta restaurar o estado.
Não existe restauração garantida após encerramento abrupto do jogo.

Não há leitura de LOCALAPPDATA, FFI, execução de shell, injeção ou alteração
das proteções do FiveM nesta integração. ESP, Aim, Bridge e demais módulos
não foram alterados. Isso não certifica o comportamento desses módulos.

## Verificação

`lua tests/skinchanger_authorized.lua` usa nativas simuladas para testar
permissão, lista aprovada, índices, falhas, troca de arma, respawn, componentes
preexistentes, interferência externa e parada do recurso. GitHub Actions
executa esses testes e compila o EXE. Testes simulados e compilação não provam
renderização das skins em jogo.

Antes de liberar um mapeamento: teste num servidor administrado, com e sem ACE,
arma compatível e incompatível, aplicar/restaurar, troca de arma, respawn,
retirada pelo inventário e substituição pelo servidor. Verifique visualmente
o componente real, além da confirmação das nativas.

Referências:
- https://docs.fivem.net/docs/scripting-manual/migrating-from-deprecated/creating-commands/
- https://docs.fivem.net/docs/developers/server-security/
- https://docs.fivem.net/docs/developers/sandbox/
