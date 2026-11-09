local approved = SkinChangerConfig.approved or {}
local lastRequest = {}

local function notify(player, message)
    TriggerClientEvent('SkinChangerLocal:notice', player, message)
end

RegisterCommand('skinlocal', function(player, args)
    if player <= 0 or not IsPlayerAceAllowed(player, 'command.skinlocal') then return end
    local now = GetGameTimer()
    if lastRequest[player] and now >= lastRequest[player] and now - lastRequest[player] < 500 then return end
    lastRequest[player] = now
    if args[1] == 'restore' and #args == 1 then
        TriggerClientEvent('SkinChangerLocal:execute', player, 'restore')
        return
    end
    local index = tonumber(args[2])
    if #args ~= 2 or args[1] ~= 'apply' or not index or index % 1 ~= 0 or index < 0 or index > 27 then
        notify(player, 'Use /skinlocal apply 0..27 ou /skinlocal restore.')
        return
    end
    local skin = approved[index]
    if type(skin) ~= 'table' or skin.enabled ~= true or skin.verified ~= true then
        notify(player, 'ID indisponivel: o administrador ainda nao aprovou o mapeamento real.')
        return
    end
    if skin.mode ~= 'tint' and skin.mode ~= 'component' then
        notify(player, 'Modo nao suportado. Add-ons dependem do inventario do servidor.')
        return
    end
    -- Nenhum hash, componente ou alvo recebido do jogador e encaminhado.
    TriggerClientEvent('SkinChangerLocal:execute', player, 'apply', skin)
end, true)

AddEventHandler('playerDropped', function()
    lastRequest[source] = nil
end)
