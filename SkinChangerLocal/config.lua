-- Indices iguais ao combo em src/Gui/Pages/SkinChanger.hpp
-- mode:
--   "component" -> GiveWeaponComponentToPed na arma equipada (packs tipo skins mode)
--   "addon"     -> troca a arma equipada por um WEAPON_ add-on local e guarda a original
--   "tint"      -> SetPedWeaponTintIndex (fallback se o pack ainda nao tiver component)
-- weaponHash / componentHash: preencha com os nomes reais dos seus .rpf (joaat string).
-- Enquanto estiverem nil, o plugin reporta "skin sem hash configurado".

SkinChangerConfig = {
    iniRelative = 'SkinChangerLocal\\client.ini',
    heartbeatMs = 2000,
    pollMs = 250,

    skins = {
        -- Angel Love
        [0]  = { label = 'Angel Love / ASSAULTRIFLE ANGEL',   mode = 'component', baseWeapon = 'WEAPON_ASSAULTRIFLE',   weaponHash = nil, componentHash = nil },
        [1]  = { label = 'Angel Love / CARBINERIFLE ANGEL',   mode = 'component', baseWeapon = 'WEAPON_CARBINERIFLE',   weaponHash = nil, componentHash = nil },
        [2]  = { label = 'Angel Love / SPECIALCARBINE ANGEL', mode = 'component', baseWeapon = 'WEAPON_SPECIALCARBINE', weaponHash = nil, componentHash = nil },
        [3]  = { label = 'Angel Love / BAT ANGEL',            mode = 'component', baseWeapon = 'WEAPON_BAT',            weaponHash = nil, componentHash = nil },
        [4]  = { label = 'Angel Love / DAGGER ANGEL',         mode = 'component', baseWeapon = 'WEAPON_DAGGER',         weaponHash = nil, componentHash = nil },
        [5]  = { label = 'Angel Love / COMBATPISTOL ANGEL',   mode = 'component', baseWeapon = 'WEAPON_COMBATPISTOL',   weaponHash = nil, componentHash = nil },
        [6]  = { label = 'Angel Love / PISTOLMK2 ANGEL',      mode = 'component', baseWeapon = 'WEAPON_PISTOL_MK2',     weaponHash = nil, componentHash = nil },
        [7]  = { label = 'Angel Love / ASSAULTSMG ANGEL',     mode = 'component', baseWeapon = 'WEAPON_ASSAULTSMG',     weaponHash = nil, componentHash = nil },
        [8]  = { label = 'Angel Love / COMPACTSMG ANGEL',     mode = 'component', baseWeapon = 'WEAPON_COMPACTRIFLE',   weaponHash = nil, componentHash = nil },
        [9]  = { label = 'Angel Love / MICROSMG ANGEL',       mode = 'component', baseWeapon = 'WEAPON_MICROSMG',       weaponHash = nil, componentHash = nil },
        [10] = { label = 'Angel Love / SMG ANGEL',            mode = 'component', baseWeapon = 'WEAPON_SMG',            weaponHash = nil, componentHash = nil },

        -- PenhaShop
        [11] = { label = 'PenhaShop / PL G3 DE AGUA',         mode = 'addon', baseWeapon = 'WEAPON_SPECIALCARBINE', weaponHash = nil, componentHash = nil },
        [12] = { label = 'PenhaShop / PL PENHASHOP AR15 NIC', mode = 'addon', baseWeapon = 'WEAPON_CARBINERIFLE',   weaponHash = nil, componentHash = nil },
        [13] = { label = 'PenhaShop / PL PISTOLA AGUA',       mode = 'addon', baseWeapon = 'WEAPON_PISTOL',         weaponHash = nil, componentHash = nil },
        [14] = { label = 'PenhaShop / PL SMG VERAO PUDDIN',   mode = 'addon', baseWeapon = 'WEAPON_SMG',            weaponHash = nil, componentHash = nil },

        -- Glock PenhaShop
        [15] = { label = 'Glock PenhaShop / GLOCK RAJADA ANIME',  mode = 'addon', baseWeapon = 'WEAPON_COMBATPISTOL', weaponHash = nil, componentHash = nil },
        [16] = { label = 'Glock PenhaShop / GLOCK RAJADA FADE',   mode = 'addon', baseWeapon = 'WEAPON_COMBATPISTOL', weaponHash = nil, componentHash = nil },
        [17] = { label = 'Glock PenhaShop / GLOCK RAJADA GRAFITE',mode = 'addon', baseWeapon = 'WEAPON_COMBATPISTOL', weaponHash = nil, componentHash = nil },
        [18] = { label = 'Glock PenhaShop / GLOCK RAJADA OURO',   mode = 'addon', baseWeapon = 'WEAPON_COMBATPISTOL', weaponHash = nil, componentHash = nil },

        -- Diversos
        [19] = { label = 'ESQUELETO / ESQUELETO', mode = 'addon', baseWeapon = 'WEAPON_ASSAULTRIFLE',   weaponHash = nil, componentHash = nil },
        [20] = { label = 'G3 / G3',               mode = 'addon', baseWeapon = 'WEAPON_SPECIALCARBINE', weaponHash = nil, componentHash = nil },
        [21] = { label = 'G36GOAT / G36GOAT',     mode = 'addon', baseWeapon = 'WEAPON_CARBINERIFLE',   weaponHash = nil, componentHash = nil },
        [22] = { label = 'ZOLP / BARRETMATA',     mode = 'addon', baseWeapon = 'WEAPON_HEAVYSNIPER',    weaponHash = nil, componentHash = nil },
        [23] = { label = 'ZOLP / BDRX15',         mode = 'addon', baseWeapon = 'WEAPON_CARBINERIFLE',   weaponHash = nil, componentHash = nil },
        [24] = { label = 'ZOLP / BRS74',          mode = 'addon', baseWeapon = 'WEAPON_ASSAULTRIFLE',   weaponHash = nil, componentHash = nil },
        [25] = { label = 'ZOLP / FALGOLDNM',      mode = 'addon', baseWeapon = 'WEAPON_ASSAULTRIFLE',   weaponHash = nil, componentHash = nil },
        [26] = { label = 'ZOLP / GGOLDLP',        mode = 'addon', baseWeapon = 'WEAPON_COMBATPISTOL',   weaponHash = nil, componentHash = nil },
        [27] = { label = 'ZOLP / M249G',          mode = 'addon', baseWeapon = 'WEAPON_COMBATMG',       weaponHash = nil, componentHash = nil },
    },
}
