# Skin Changer local

## Arquivos

| Caminho | Papel |
|---|---|
| `src/Gui/Pages/SkinChanger.hpp` | UI (inalterada na logica de envio de comando) |
| `SkinChangerLocal/fxmanifest.lua` | Resource client-only |
| `SkinChangerLocal/config.lua` | Catalogo indexado igual ao combo da UI |
| `SkinChangerLocal/client.lua` | Heartbeat, leitura de comando, apply/restore locais |
| `SkinChangerLocal/stream/` | Destino dos modelos/texturas dos packs |

## Contrato INI

Arquivo: `%LOCALAPPDATA%\\SkinChangerLocal\\client.ini`

```
[renderer]
heartbeat=<GetTickCount64 ms>
status=<mensagem para a UI>
command=<timestamp>:<indice>   ; indice -1 = restaurar
```

A UI ja escreve `command` e le `heartbeat`/`status`. O resource completa o lado do jogo.

## O que nao foi alterado

Aimbot, ESP, bridge nativa, menus, design, build do .exe e demais features permanecem iguais. Nenhum arquivo fora de `SkinChangerLocal/` e desta doc foi modificado nesta entrega.

## Testes feitos neste ambiente

- Leitura da UI e confirmacao do caminho INI
- Criacao do resource e do catalogo alinhado aos 28 itens do combo
- Nao houve execucao dentro do FiveM nem compilacao do EXE neste passo (resource Lua puro, fora do pipeline MSBuild)
