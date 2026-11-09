local events, handlers, messages = {}, {}, {}
local ped, selected, tint, owned, components, failTint, failComponent
local names = { WEAPON_TEST = 10, WEAPON_OTHER = 20, WEAPON_UNARMED = 0, COMP_A = 100, COMP_B = 101 }
function RegisterNetEvent(name, callback) events[name] = callback end
function AddEventHandler(name, callback) handlers[name] = callback end
function TriggerEvent(_, data) messages[#messages + 1] = data.args[2] end
function PlayerPedId() return ped end
function DoesEntityExist(value) return value == ped end
function IsEntityDead() return false end
function GetHashKey(name) return names[name] or 999 end
function IsWeaponValid(weapon) return weapon == 10 or weapon == 20 end
function GetSelectedPedWeapon() return selected end
function HasPedGotWeapon(_, weapon) return owned[weapon] == true end
function GetWeaponTintCount() return 8 end
function GetPedWeaponTintIndex(_, weapon) return tint[weapon] end
function SetPedWeaponTintIndex(_, weapon, value) if not failTint then tint[weapon] = value end end
function DoesWeaponTakeWeaponComponent(weapon, component)
    return weapon == 10 and (component == 100 or component == 101)
end
function HasPedGotWeaponComponent(_, weapon, component) return components[weapon][component] == true end
function GiveWeaponComponentToPed(_, weapon, component)
    if failComponent then return end
    components[weapon][100], components[weapon][101] = false, false
    components[weapon][component] = true
end
function RemoveWeaponComponentFromPed(_, weapon, component) components[weapon][component] = false end
function GetCurrentResourceName() return 'SkinChangerLocal' end

local function reset()
    ped, selected = 1, 10
    tint, owned = { [10] = 2, [20] = 5 }, { [10] = true, [20] = true }
    components = { [10] = { [100] = true }, [20] = {} }
    failTint, failComponent = false, false
    events, handlers, messages = {}, {}, {}
    source = 65535
    dofile('SkinChangerLocal/client.lua')
end
local function run(action, skin) events['SkinChangerLocal:execute'](action, skin) end
local skin = { enabled = true, verified = true, mode = 'tint', baseWeapon = 'WEAPON_TEST', tintIndex = 3 }
local component = { enabled = true, verified = true, mode = 'component', baseWeapon = 'WEAPON_TEST',
    componentHash = 'COMP_B', componentGroupVerified = true, componentGroup = { 'COMP_A', 'COMP_B' } }

reset()
source = 1
run('apply', skin)
assert(tint[10] == 2 and #messages == 0, 'local event must be rejected')
source = 65535
run('apply', skin)
assert(tint[10] == 3)
selected = 20
run('restore')
assert(tint[10] == 2 and tint[20] == 5, 'restore must target saved weapon')

reset()
failTint = true
run('apply', skin)
assert(tint[10] == 2 and messages[#messages]:match('Erro:'), 'failed native must not report success')

reset()
run('apply', skin)
tint[10] = 6
run('restore')
assert(tint[10] == 6 and messages[#messages]:match('externamente'), 'external tint must be preserved')

reset()
run('apply', skin)
ped = 2
run('restore')
assert(tint[10] == 3 and messages[#messages]:match('indisponivel'), 'respawn must not restore onto new ped')

reset()
run('apply', component)
assert(components[10][101] and not components[10][100])
selected = 20
run('restore')
assert(components[10][100] and not components[10][101], 'conflicting original component must return')

reset()
components[10] = { [101] = true }
run('apply', component)
run('restore')
assert(components[10][101], 'preexisting requested component must survive restoration')

reset()
failComponent = true
run('apply', component)
assert(components[10][100] and messages[#messages]:match('Erro:'), 'failed component must not report success')

reset()
run('apply', component)
components[10] = { [100] = true }
run('restore')
assert(components[10][100] and messages[#messages]:match('externamente'))

reset()
run('apply', skin)
owned[10] = false
run('restore')
assert(not owned[10] and messages[#messages]:match('indisponivel'), 'removed weapon must not be recreated')

reset()
run('apply', skin)
handlers.onClientResourceStop('SkinChangerLocal')
assert(tint[10] == 2, 'resource stop must restore')

reset()
skin.tintIndex = 99
run('apply', skin)
assert(tint[10] == 2 and messages[#messages]:match('Erro:'))
skin.tintIndex = 3
component.componentGroup = { 'COMP_A' }
run('apply', component)
assert(components[10][100] and messages[#messages]:match('Erro:'))
component.componentGroup = { 'COMP_A', 'COMP_B' }

-- Malformed mappings cannot reach a native mutation.
reset()
skin.baseWeapon = 'WEAPON_TEST invalid'
run('apply', skin)
assert(tint[10] == 2 and messages[#messages]:match('Erro:'))
skin.baseWeapon = 'WEAPON_TEST'
component.componentGroup = { [1] = 'COMP_A', [3] = 'COMP_B' }
run('apply', component)
assert(components[10][100] and messages[#messages]:match('Erro:'))
component.componentGroup = { 'COMP_A', 'COMP_B', extra = 'COMP_A' }
run('apply', component)
assert(components[10][100] and messages[#messages]:match('Erro:'))
component.componentGroup = { 'COMP_A', 'COMP_B' }
reset()
tint[10] = 99
run('apply', skin)
assert(tint[10] == 99 and messages[#messages]:match('Erro:'))
reset()
run('apply', skin)
local normalSetTint = SetPedWeaponTintIndex
SetPedWeaponTintIndex = function(_, weapon) tint[weapon] = 4 end
run('restore')
assert(tint[10] == 4 and messages[#messages]:match('Erro:'))
SetPedWeaponTintIndex = normalSetTint
run('restore')
assert(tint[10] == 2 and messages[#messages]:match('OK:'), 'partial restore must remain retryable')

-- Server authorization and allow-list behavior, independent of client mocks.
local command, restricted, sent, allowed, now = nil, nil, {}, false, 1000
function RegisterCommand(name, fn, flag) assert(name == 'skinlocal'); command, restricted = fn, flag end
function IsPlayerAceAllowed(_, ace) assert(ace == 'command.skinlocal'); return allowed end
function GetGameTimer() return now end
function TriggerClientEvent(name, player, action, value)
    sent[#sent + 1] = { name = name, player = player, action = action, value = value }
end
SkinChangerConfig = { approved = { [0] = skin } }
dofile('SkinChangerLocal/server.lua')
assert(restricted == true)
command(7, { 'apply', '0' })
assert(#sent == 0, 'permission denied must not emit any action')
allowed = true
command(7, { 'apply', '0' })
assert(sent[1].name == 'SkinChangerLocal:execute' and sent[1].value == skin and sent[1].player == 7)
command(7, { 'apply', '0' })
assert(#sent == 1, 'rate limit')
now = now + 1000
command(7, { 'apply', '28' })
assert(sent[#sent].name == 'SkinChangerLocal:notice')
now = now + 1000
command(7, { 'apply', '1' })
assert(sent[#sent].name == 'SkinChangerLocal:notice', 'unapproved entry must be rejected')
now = now + 1000
command(7, { 'restore' })
assert(sent[#sent].name == 'SkinChangerLocal:execute' and sent[#sent].action == 'restore')
now = now + 1000
skin.mode = 'addon'
command(7, { 'apply', '0' })
assert(sent[#sent].name == 'SkinChangerLocal:notice')
dofile('SkinChangerLocal/config.lua')
assert(next(SkinChangerConfig.approved) == nil, 'unverified catalogue must default to disabled')
local count = 0
for id in pairs(SkinChangerConfig.skins) do assert(id >= 0 and id <= 27); count = count + 1 end
assert(count == 28, 'catalogue IDs must be preserved')
for id = 0, 27 do assert(type(SkinChangerConfig.skins[id]) == 'table') end
assert(SkinChangerConfig.skins[8].baseWeapon == nil, 'unknown SMG mapping must not guess a rifle')
print('SkinChanger authorization, failure and restoration tests passed.')
