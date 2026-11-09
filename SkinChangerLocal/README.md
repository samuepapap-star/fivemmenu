# SkinChangerLocal

Plugin local usado pela pagina `src/Gui/Pages/SkinChanger.hpp` do Wexize.

## O que faz

- Escreve heartbeat em `%LOCALAPPDATA%\\SkinChangerLocal\\client.ini`
- Le o comando `timestamp:indice` enviado pelo menu
- Aplica skin **somente neste cliente** (sem evento de rede)
- Restaura arma/componentes originais no botao Restaurar

## Instalacao

1. Copie a pasta `SkinChangerLocal` para `resources/` do servidor FiveM (ou cliente local se a cidade permitir resource local).
2. Extraia os packs Angel Love / PenhaShop / Glocks / etc. e coloque modelos e texturas em `SkinChangerLocal/stream/`.
3. Se o pack trouxer `.meta`, coloque em `SkinChangerLocal/data/` e descomente os `data_file` no `fxmanifest.lua`.
4. Em `config.lua`, preencha `weaponHash` e/ou `componentHash` com os nomes reais do pack (ex.: `WEAPON_...`, `COMPONENT_...`).
5. No `server.cfg`:

```
ensure SkinChangerLocal
```

6. Abra o Wexize, va na pagina Skin Changer. O botao deixa de ficar desabilitado quando o heartbeat estiver vivo.

## Indices da UI

Os indices em `config.lua` seguem a ordem do combo em `SkinChanger.hpp` (0 = primeira skin da lista).

## Limitacao atual

Os hashes reais dos seus `.rpf` nao estavam no repositorio. Enquanto `weaponHash` / `componentHash` forem `nil`, o plugin responde com status pedindo o preenchimento — a ponte UI↔plugin ja funciona.
