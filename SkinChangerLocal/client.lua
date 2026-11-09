local cfg = SkinChangerConfig
local iniPath
local lastCommand = ''
local applied = {
    index = -1,
    mode = nil,
    originalWeapon = 0,
    originalAmmo = 0,
    originalTint = 0,
    componentHash = 0,
    addonWeapon = 0,
}

local function joaat(name)
    if not name or name == '' then return 0 end
    return GetHashKey(name)
end

local function systemTickMs()
    local ok, result = pcall(function()
        if not _G.__skin_tick then
            local ffi = require('ffi')
            ffi.cdef[[
                typedef unsigned long long ULONGLONG;
                ULONGLONG __stdcall GetTickCount64(void);
            ]]
            _G.__skin_tick = ffi.C.GetTickCount64
        end
        return tonumber(tostring(_G.__skin_tick()))
    end)
    if ok and result and result > 0 then
        return result
    end
    return (os.time() * 1000) + (GetGameTimer() % 1000)
end

local function ensureIni()
    if iniPath then return iniPath end
    local appdata = nil
    pcall(function()
        if GetEnv then appdata = GetEnv('LOCALAPPDATA') end
    end)
    if (not appdata or appdata == '') and os.getenv then
        appdata = os.getenv('LOCALAPPDATA')
    end
    if not appdata or appdata == '' then
        return nil
    end
    iniPath = appdata .. '\\SkinChangerLocal\\client.ini'
    return iniPath
end

local function writeIni(section, key, value)
    local path = ensureIni()
    if not path then return false end
    local ok = pcall(function()
        local f = io.open(path, 'r')
        local content = f and f:read('*a') or ''
        if f then f:close() end

        local sectionHeader = '[' .. section .. ']'
        local lines = {}
        local inSection = false
        local foundSection = false
        local foundKey = false

        for line in (content .. '\n'):gmatch('(.-)\n') do
            local header = line:match('^%[(.-)%]$')
            if header then
                if inSection and not foundKey then
                    lines[#lines + 1] = key .. '=' .. tostring(value)
                    foundKey = true
                end
                inSection = (header:lower() == section:lower())
                if inSection then foundSection = true end
                lines[#lines + 1] = line
            elseif inSection then
                local k = line:match('^([^=]+)=')
                if k and k:lower() == key:lower() then
                    lines[#lines + 1] = key .. '=' .. tostring(value)
                    foundKey = true
                else
                    lines[#lines + 1] = line
                end
            else
                lines[#lines + 1] = line
            end
        end

        if inSection and not foundKey then
            lines[#lines + 1] = key .. '=' .. tostring(value)
            foundKey = true
        end

        if not foundSection then
            lines[#lines + 1] = sectionHeader
            lines[#lines + 1] = key .. '=' .. tostring(value)
        end

        local out = table.concat(lines, '\n')
        if not out:match('\n$') then out = out .. '\n' end

        local dir = path:match('^(.+)\\[^\\]+$')
        if dir then
            os.execute('mkdir "' .. dir .. '" 2>nul')
        end

        local w = io.open(path, 'w')
        if not w then error('open write failed') end
        w:write(out)
        w:close()
    end)
    return ok
end

local function readIni(section, key)
    local path = ensureIni()
    if not path then return '' end
    local ok, value = pcall(function()
        local f = io.open(path, 'r')
        if not f then return '' end
        local content = f:read('*a') or ''
        f:close()
        local inSection = false
        for line in (content .. '\n'):gmatch('(.-)\n') do
            local header = line:match('^%[(.-)%]$')
            if header then
                inSection = (header:lower() == section:lower())
            elseif inSection then
                local k, v = line:match('^([^=]+)=(.*)$')
                if k and k:lower() == key:lower() then
                    return (v or ''):gsub('%s+$', '')
                end
            end
        end
        return ''
    end)
    if ok then return value or '' end
    return ''
end

local function setStatus(msg)
    writeIni('renderer', 'status', msg or '')
end

local function setHeartbeat()
    writeIni('renderer', 'heartbeat', tostring(systemTickMs()))
end

local function currentWeapon(ped)
    local _, weaponHash = GetCurrentPedWeapon(ped, true)
    if not weaponHash or weaponHash == 0 or weaponHash == `WEAPON_UNARMED` then
        return 0
    end
    return weaponHash
end

local function clearAppliedComponents(ped, weapon)
    if applied.componentHash ~= 0 and HasPedGotWeaponComponent(ped, weapon, applied.componentHash) then
        RemoveWeaponComponentFromPed(ped, weapon, applied.componentHash)
    end
end

local function waitWeaponAsset(hash, timeoutMs)
    if HasWeaponAssetLoaded(hash) then return true end
    RequestWeaponAsset(hash, 31, 0)
    local deadline = GetGameTimer() + (timeoutMs or 2500)
    while not HasWeaponAssetLoaded(hash) and GetGameTimer() < deadline do
        Wait(0)
    end
    return HasWeaponAssetLoaded(hash)
end

local function restoreOriginal()
    local ped = PlayerPedId()
    if not DoesEntityExist(ped) then
        setStatus('Personagem indisponivel.')
        return
    end

    local weapon = currentWeapon(ped)

    if applied.mode == 'component' and weapon ~= 0 then
        clearAppliedComponents(ped, weapon)
        if applied.originalTint and applied.originalTint >= 0 then
            SetPedWeaponTintIndex(ped, weapon, applied.originalTint)
        end
        setStatus('Skin de componente restaurada na arma equipada.')
    elseif applied.mode == 'addon' and applied.originalWeapon ~= 0 then
        local ammo = applied.originalAmmo or 0
        if applied.addonWeapon ~= 0 and HasPedGotWeapon(ped, applied.addonWeapon, false) then
            RemoveWeaponFromPed(ped, applied.addonWeapon)
        end
        waitWeaponAsset(applied.originalWeapon, 1500)
        if not HasPedGotWeapon(ped, applied.originalWeapon, false) then
            GiveWeaponToPed(ped, applied.originalWeapon, ammo, false, true)
        else
            SetPedAmmo(ped, applied.originalWeapon, ammo)
        end
        SetCurrentPedWeapon(ped, applied.originalWeapon, true)
        if applied.originalTint and applied.originalTint >= 0 then
            SetPedWeaponTintIndex(ped, applied.originalWeapon, applied.originalTint)
        end
        setStatus('Arma original restaurada localmente.')
    elseif applied.mode == 'tint' and weapon ~= 0 then
        SetPedWeaponTintIndex(ped, weapon, applied.originalTint or 0)
        setStatus('Tint original restaurado.')
    else
        setStatus('Nenhuma skin ativa para restaurar.')
    end

    applied.index = -1
    applied.mode = nil
    applied.originalWeapon = 0
    applied.originalAmmo = 0
    applied.originalTint = 0
    applied.componentHash = 0
    applied.addonWeapon = 0
end

local function applySkin(index)
    local skin = cfg.skins[index]
    if not skin then
        setStatus('Indice de skin invalido: ' .. tostring(index))
        return
    end

    local ped = PlayerPedId()
    if not DoesEntityExist(ped) then
        setStatus('Personagem indisponivel.')
        return
    end

    local weapon = currentWeapon(ped)
    if weapon == 0 then
        setStatus('Equipe uma arma antes de aplicar a skin.')
        return
    end

    if applied.mode then
        restoreOriginal()
        Wait(50)
        weapon = currentWeapon(ped)
        if weapon == 0 then
            setStatus('Arma perdida apos restaurar a anterior.')
            return
        end
    end

    local mode = skin.mode or 'component'
    local ammo = GetAmmoInPedWeapon(ped, weapon)
    local tint = GetPedWeaponTintIndex(ped, weapon)

    if mode == 'component' then
        if not skin.componentHash then
            setStatus('Skin "' .. skin.label .. '" sem componentHash. Preencha config.lua.')
            return
        end
        local component = joaat(skin.componentHash)
        if component == 0 then
            setStatus('componentHash invalido em config.lua')
            return
        end
        if not DoesWeaponTakeWeaponComponent(weapon, component) then
            setStatus('Esta arma nao aceita o component. Equipe ' .. (skin.baseWeapon or '?') .. '.')
            return
        end

        applied.mode = 'component'
        applied.index = index
        applied.originalWeapon = weapon
        applied.originalAmmo = ammo
        applied.originalTint = tint
        applied.componentHash = component
        applied.addonWeapon = 0

        GiveWeaponComponentToPed(ped, weapon, component)
        setStatus('Skin aplicada (component): ' .. skin.label)
        return
    end

    if mode == 'addon' then
        if not skin.weaponHash then
            setStatus('Skin "' .. skin.label .. '" sem weaponHash. Preencha config.lua.')
            return
        end
        local addon = joaat(skin.weaponHash)
        if addon == 0 then
            setStatus('weaponHash invalido em config.lua')
            return
        end

        if not waitWeaponAsset(addon, 3000) then
            setStatus('Modelo nao carregou: ' .. skin.weaponHash .. '. Coloque o pack em stream/ e ensure SkinChangerLocal (ou o resource do pack).')
            return
        end

        applied.mode = 'addon'
        applied.index = index
        applied.originalWeapon = weapon
        applied.originalAmmo = ammo
        applied.originalTint = tint
        applied.componentHash = 0
        applied.addonWeapon = addon

        RemoveWeaponFromPed(ped, weapon)
        GiveWeaponToPed(ped, addon, ammo > 0 and ammo or 120, false, true)
        SetCurrentPedWeapon(ped, addon, true)
        setStatus('Skin aplicada (addon local): ' .. skin.label)
        return
    end

    if mode == 'tint' then
        local tintIndex = tonumber(skin.tintIndex) or 0
        applied.mode = 'tint'
        applied.index = index
        applied.originalWeapon = weapon
        applied.originalAmmo = ammo
        applied.originalTint = tint
        applied.componentHash = 0
        applied.addonWeapon = 0
        SetPedWeaponTintIndex(ped, weapon, tintIndex)
        setStatus('Tint aplicado: ' .. skin.label)
        return
    end

    setStatus('Mode desconhecido em config.lua: ' .. tostring(mode))
end

local function handleCommand(raw)
    if not raw or raw == '' or raw == lastCommand then
        return
    end
    lastCommand = raw

    local ts, idx = raw:match('^(%d+):(%-?%d+)$')
    if not ts then
        setStatus('Comando invalido do menu: ' .. tostring(raw))
        return
    end

    local index = tonumber(idx)
    if index == -1 then
        restoreOriginal()
        return
    end

    applySkin(index)
end

CreateThread(function()
    setStatus('Plugin local ativo. Aguardando comando do Wexize.')
    while true do
        setHeartbeat()
        local command = readIni('renderer', 'command')
        handleCommand(command)
        Wait(cfg.pollMs or 250)
    end
end)

AddEventHandler('onResourceStop', function(res)
    if res ~= GetCurrentResourceName() then return end
    if applied.mode then
        restoreOriginal()
    end
    setStatus('Plugin local parado.')
end)
