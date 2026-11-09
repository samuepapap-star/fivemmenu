local applied

local function notify(message)
    print('[SkinChangerLocal] ' .. message)
    TriggerEvent('chat:addMessage', { args = { 'SkinChangerLocal', message } })
end

local function integer(value)
    return type(value) == 'number' and value == value and value % 1 == 0
end

local function hash(name)
    if type(name) ~= 'string' or #name == 0 or #name > 100 then return nil end
    return GetHashKey(name)
end

local function restore()
    if not applied then return true, 'Nenhuma alteracao ativa.' end
    local state = applied
    -- Nunca restaura em outro personagem ou recria uma arma retirada pelo servidor.
    if PlayerPedId() ~= state.ped or not DoesEntityExist(state.ped)
        or not HasPedGotWeapon(state.ped, state.weapon, false) then
        applied = nil
        return false, 'Restauracao cancelada: personagem ou arma original indisponivel.'
    end
    if state.mode == 'tint' then
        if GetPedWeaponTintIndex(state.ped, state.weapon) ~= state.target then
            applied = nil
            return false, 'Tint alterado externamente; nenhuma sobrescrita realizada.'
        end
        SetPedWeaponTintIndex(state.ped, state.weapon, state.original)
        if GetPedWeaponTintIndex(state.ped, state.weapon) ~= state.original then
            return false, 'Restauracao do tint nao confirmada; tente novamente.'
        end
    else
        -- Detecta alteracao concorrente no grupo antes de tocar nos componentes.
        for _, item in ipairs(state.components) do
            if HasPedGotWeaponComponent(state.ped, state.weapon, item.hash) ~= item.after then
                applied = nil
                return false, 'Componentes alterados externamente; nenhuma sobrescrita realizada.'
            end
        end
        for _, item in ipairs(state.components) do
            if not item.before then RemoveWeaponComponentFromPed(state.ped, state.weapon, item.hash) end
        end
        for _, item in ipairs(state.components) do
            if item.before then GiveWeaponComponentToPed(state.ped, state.weapon, item.hash) end
        end
        local restored = true
        for _, item in ipairs(state.components) do
            item.after = HasPedGotWeaponComponent(state.ped, state.weapon, item.hash)
            if item.after ~= item.before then restored = false end
        end
        if not restored then return false, 'Restauracao dos componentes nao confirmada; tente novamente.' end
    end
    applied = nil
    return true, 'Estado original confirmado na arma original.'
end

local function apply(skin)
    if type(skin) ~= 'table' or skin.enabled ~= true or skin.verified ~= true then
        return false, 'Mapeamento nao aprovado.'
    end
    local ped = PlayerPedId()
    local weapon = hash(skin.baseWeapon)
    if not weapon or not DoesEntityExist(ped) or IsEntityDead(ped)
        or weapon == GetHashKey('WEAPON_UNARMED')
        or GetSelectedPedWeapon(ped) ~= weapon
        or not HasPedGotWeapon(ped, weapon, false) then
        return false, 'Equipe a arma registrada pelo administrador.'
    end
    local target, components
    if skin.mode == 'tint' then
        target = skin.tintIndex
        if not integer(target) or target < 0 or target >= GetWeaponTintCount(weapon) then
            return false, 'Tint invalido para esta arma.'
        end
    elseif skin.mode == 'component' then
        target = hash(skin.componentHash)
        if not target or not DoesWeaponTakeWeaponComponent(weapon, target)
            or skin.componentGroupVerified ~= true
            or type(skin.componentGroup) ~= 'table'
            or #skin.componentGroup < 1 or #skin.componentGroup > 64 then
            return false, 'Componente ou grupo de restauracao nao validado.'
        end
        components = {}
        local seen, containsTarget = {}, false
        for _, name in ipairs(skin.componentGroup) do
            local value = hash(name)
            if not value or seen[value] or not DoesWeaponTakeWeaponComponent(weapon, value) then
                return false, 'Grupo de componentes invalido.'
            end
            seen[value] = true
            containsTarget = containsTarget or value == target
            components[#components + 1] = { hash = value }
        end
        if not containsTarget then return false, 'Componente ausente no grupo de restauracao.' end
    else
        return false, 'Modo nao suportado.'
    end
    if applied then
        local ok, message = restore()
        if not ok then return false, message end
    end
    -- Sem Wait: captura, mutacao e leitura ocorrem na mesma execucao.
    if skin.mode == 'tint' then
        local original = GetPedWeaponTintIndex(ped, weapon)
        if original < 0 then return false, 'Nao foi possivel capturar o tint original.' end
        SetPedWeaponTintIndex(ped, weapon, target)
        local actual = GetPedWeaponTintIndex(ped, weapon)
        if actual ~= target then
            if actual ~= original then
                applied = { ped = ped, weapon = weapon, mode = 'tint', original = original, target = actual }
                restore()
            end
            return false, 'Aplicacao do tint nao confirmada.'
        end
        applied = { ped = ped, weapon = weapon, mode = 'tint', original = original, target = target }
    else
        for _, item in ipairs(components) do
            item.before = HasPedGotWeaponComponent(ped, weapon, item.hash)
        end
        GiveWeaponComponentToPed(ped, weapon, target)
        for _, item in ipairs(components) do
            item.after = HasPedGotWeaponComponent(ped, weapon, item.hash)
        end
        applied = { ped = ped, weapon = weapon, mode = 'component', components = components }
        if not HasPedGotWeaponComponent(ped, weapon, target) then
            local ok = restore()
            return false, ok and 'Aplicacao recusada; estado anterior confirmado.'
                or 'Aplicacao e restauracao nao confirmadas; use restore.'
        end
    end
    return true, 'Aplicacao confirmada pelo cliente; o servidor pode substituir o estado depois.'
end

RegisterNetEvent('SkinChangerLocal:notice', function(message)
    if source ~= 65535 or type(message) ~= 'string' then return end
    notify(message)
end)

RegisterNetEvent('SkinChangerLocal:execute', function(action, skin)
    if source ~= 65535 then return end
    local ok, message
    if action == 'restore' then ok, message = restore()
    elseif action == 'apply' then ok, message = apply(skin)
    else return end
    notify((ok and 'OK: ' or 'Erro: ') .. message)
end)

AddEventHandler('onClientResourceStop', function(resource)
    if resource ~= GetCurrentResourceName() then return end
    local _, message = restore()
    print('[SkinChangerLocal] ' .. message)
end)
