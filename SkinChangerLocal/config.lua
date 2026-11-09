-- Indices iguais ao combo em src/Gui/Pages/SkinChanger.hpp
-- mode:
--   "component" -> GiveWeaponComponentToPed na arma equipada
--   "addon"     -> troca local por WEAPON_ add-on e guarda a original
--   "tint"      -> SetPedWeaponTintIndex
--
-- Os nomes abaixo seguem convencao dos packs BR (Angel Love / PenhaShop / ZOLP).
-- Se o spawn name do SEU .rpf for diferente, edite so esta tabela.
-- O client tenta o hash; se o modelo nao estiver streamado, o status avisa.

SkinChangerConfig = {
    iniRelative = 'SkinChangerLocal\\client.ini',
    heartbeatMs = 2000,
    pollMs = 250,

    skins = {
        -- Angel Love (addons com spawn name tipico do pack)
        [0]  = { label = 'Angel Love / ASSAULTRIFLE ANGEL',   mode = 'addon', baseWeapon = 'WEAPON_ASSAULTRIFLE',   weaponHash = 'WEAPON_ASSAULTRIFLE_ANGEL',   componentHash = nil },
        [1]  = { label = 'Angel Love / CARBINERIFLE ANGEL',   mode = 'addon', baseWeapon = 'WEAPON_CARBINERIFLE',   weaponHash = 'WEAPON_CARBINERIFLE_ANGEL',   componentHash = nil },
        [2]  = { label = 'Angel Love / SPECIALCARBINE ANGEL', mode = 'addon', baseWeapon = 'WEAPON_SPECIALCARBINE', weaponHash = 'WEAPON_SPECIALCARBINE_ANGEL', componentHash = nil },
        [3]  = { label = 'Angel Love / BAT ANGEL',            mode = 'addon', baseWeapon = 'WEAPON_BAT',            weaponHash = 'WEAPON_BAT_ANGEL',            componentHash = nil },
        [4]  = { label = 'Angel Love / DAGGER ANGEL',         mode = 'addon', baseWeapon = 'WEAPON_DAGGER',         weaponHash = 'WEAPON_DAGGER_ANGEL',         componentHash = nil },
        [5]  = { label = 'Angel Love / COMBATPISTOL ANGEL',   mode = 'addon', baseWeapon = 'WEAPON_COMBATPISTOL',   weaponHash = 'WEAPON_COMBATPISTOL_ANGEL',   componentHash = nil },
        [6]  = { label = 'Angel Love / PISTOLMK2 ANGEL',      mode = 'addon', baseWeapon = 'WEAPON_PISTOL_MK2',     weaponHash = 'WEAPON_PISTOLMK2_ANGEL',      componentHash = nil },
        [7]  = { label = 'Angel Love / ASSAULTSMG ANGEL',     mode = 'addon', baseWeapon = 'WEAPON_ASSAULTSMG',     weaponHash = 'WEAPON_ASSAULTSMG_ANGEL',     componentHash = nil },
        [8]  = { label = 'Angel Love / COMPACTSMG ANGEL',     mode = 'addon', baseWeapon = 'WEAPON_COMPACTRIFLE',   weaponHash = 'WEAPON_COMPACTSMG_ANGEL',     componentHash = nil },
        [9]  = { label = 'Angel Love / MICROSMG ANGEL',       mode = 'addon', baseWeapon = 'WEAPON_MICROSMG',       weaponHash = 'WEAPON_MICROSMG_ANGEL',       componentHash = nil },
        [10] = { label = 'Angel Love / SMG ANGEL',            mode = 'addon', baseWeapon = 'WEAPON_SMG',            weaponHash = 'WEAPON_SMG_ANGEL',            componentHash = nil },

        -- PenhaShop
        [11] = { label = 'PenhaShop / PL G3 DE AGUA',         mode = 'addon', baseWeapon = 'WEAPON_SPECIALCARBINE', weaponHash = 'WEAPON_PL_G3_DE_AGUA',         componentHash = nil },
        [12] = { label = 'PenhaShop / PL PENHASHOP AR15 NIC', mode = 'addon', baseWeapon = 'WEAPON_CARBINERIFLE',   weaponHash = 'WEAPON_PL_PENHASHOP_AR15_NIC', componentHash = nil },
        [13] = { label = 'PenhaShop / PL PISTOLA AGUA',       mode = 'addon', baseWeapon = 'WEAPON_PISTOL',         weaponHash = 'WEAPON_PL_PISTOLA_AGUA',       componentHash = nil },
        [14] = { label = 'PenhaShop / PL SMG VERAO PUDDIN',   mode = 'addon', baseWeapon = 'WEAPON_SMG',            weaponHash = 'WEAPON_PL_SMG_VERAO_PUDDIN',   componentHash = nil },

        -- Glock PenhaShop
        [15] = { label = 'Glock PenhaShop / GLOCK RAJADA ANIME',   mode = 'addon', baseWeapon = 'WEAPON_COMBATPISTOL', weaponHash = 'WEAPON_GLOCK_RAJADA_ANIME',   componentHash = nil },
        [16] = { label = 'Glock PenhaShop / GLOCK RAJADA FADE',    mode = 'addon', baseWeapon = 'WEAPON_COMBATPISTOL', weaponHash = 'WEAPON_GLOCK_RAJADA_FADE',    componentHash = nil },
        [17] = { label = 'Glock PenhaShop / GLOCK RAJADA GRAFITE', mode = 'addon', baseWeapon = 'WEAPON_COMBATPISTOL', weaponHash = 'WEAPON_GLOCK_RAJADA_GRAFITE', componentHash = nil },
        [18] = { label = 'Glock PenhaShop / GLOCK RAJADA OURO',    mode = 'addon', baseWeapon = 'WEAPON_COMBATPISTOL', weaponHash = 'WEAPON_GLOCK_RAJADA_OURO',    componentHash = nil },

        -- Diversos
        [19] = { label = 'ESQUELETO / ESQUELETO', mode = 'addon', baseWeapon = 'WEAPON_ASSAULTRIFLE',   weaponHash = 'WEAPON_ESQUELETO',  componentHash = nil },
        [20] = { label = 'G3 / G3',               mode = 'addon', baseWeapon = 'WEAPON_SPECIALCARBINE', weaponHash = 'WEAPON_G3',         componentHash = nil },
        [21] = { label = 'G36GOAT / G36GOAT',     mode = 'addon', baseWeapon = 'WEAPON_CARBINERIFLE',   weaponHash = 'WEAPON_G36GOAT',    componentHash = nil },
        [22] = { label = 'ZOLP / BARRETMATA',     mode = 'addon', baseWeapon = 'WEAPON_HEAVYSNIPER',    weaponHash = 'WEAPON_BARRETMATA', componentHash = nil },
        [23] = { label = 'ZOLP / BDRX15',         mode = 'addon', baseWeapon = 'WEAPON_CARBINERIFLE',   weaponHash = 'WEAPON_BDRX15',     componentHash = nil },
        [24] = { label = 'ZOLP / BRS74',          mode = 'addon', baseWeapon = 'WEAPON_ASSAULTRIFLE',   weaponHash = 'WEAPON_BRS74',      componentHash = nil },
        [25] = { label = 'ZOLP / FALGOLDNM',      mode = 'addon', baseWeapon = 'WEAPON_ASSAULTRIFLE',   weaponHash = 'WEAPON_FALGOLDNM',  componentHash = nil },
        [26] = { label = 'ZOLP / GGOLDLP',        mode = 'addon', baseWeapon = 'WEAPON_COMBATPISTOL',   weaponHash = 'WEAPON_GGOLDLP',    componentHash = nil },
        [27] = { label = 'ZOLP / M249G',          mode = 'addon', baseWeapon = 'WEAPON_COMBATMG',       weaponHash = 'WEAPON_M249G',      componentHash = nil },
    },
}
