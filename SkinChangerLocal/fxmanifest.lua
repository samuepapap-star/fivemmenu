fx_version 'cerulean'
game 'gta5'

name 'SkinChangerLocal'
description 'Plugin local do Skin Changer do Wexize. Aplica e restaura skins somente no cliente.'
version '1.0.0'

-- Resource client-only: sem server scripts e sem sync de rede.
client_scripts {
    'config.lua',
    'client.lua',
}

-- Pasta stream pronta para receber .ydr/.ytd/.ycd dos packs Angel Love, PenhaShop, Glocks, etc.
-- Coloque os arquivos extraidos dos .rpf em stream/ e ajuste config.lua (weaponHash/componentHash).
files {
    'stream/**/*',
}

-- data_file entries ficam comentadas ate voce ter os .meta dos packs.
-- data_file 'WEAPONINFO_FILE' 'data/weapons.meta'
-- data_file 'WEAPONCOMPONENTSINFO_FILE' 'data/weaponcomponents.meta'
-- data_file 'WEAPON_METADATA_FILE' 'data/weaponarchetypes.meta'
