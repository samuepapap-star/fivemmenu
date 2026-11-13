#pragma once
#include <Core/SDK/SDK.hpp>
#include <Core/Threads/VehicleList.hpp>
#include <cmath>
#include <thread>
#include <chrono>
#include <Core/Features/XrcVehiclePull.hpp>
#include <Bridge/Features.hpp>

// XRC Updated vehicle pull + SHARK overhead grab movement.
// XRC pull uses its game network-control fields; no driver ejection or native injection.
namespace Core { namespace Features { namespace VehicleManipulation {
inline bool enabled = false;
inline float height = 5.f, throwSpeed = 60.f;
inline uintptr_t held = 0, heldModel = 0, heldPed = 0;
inline DWORD heldPid = 0;
inline bool keyLatch = false;
inline const char* status = "Desativado";
template<class T> bool Read(uintptr_t a, T& v) {
    SIZE_T n = 0;
    return a >= 0x10000 && Mem.ProcHandle &&
        ReadProcessMemory(Mem.ProcHandle, (LPCVOID)a, &v, sizeof(v), &n) && n == sizeof(v);
}
template<class T> bool Write(uintptr_t a, const T& v) {
    SIZE_T n = 0;
    return a >= 0x10000 && Mem.ProcHandle &&
        WriteProcessMemory(Mem.ProcHandle, (LPVOID)a, &v, sizeof(v), &n) && n == sizeof(v);
}
inline bool Sane(D3DXVECTOR3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) &&
        std::abs(v.x) < 100000.f && std::abs(v.y) < 100000.f && std::abs(v.z) < 100000.f;
}
inline bool Listed(uintptr_t address) {
    if (address < 0x10000) return false;
    std::lock_guard<std::mutex> lock(Core::Threads::g_VehicleList.vehicleListMutex);
    for (const auto& v : SDK::Game::VehicleList)
        if (reinterpret_cast<uintptr_t>(v.Pointer) == address) return true;
    return false;
}
inline bool Player(uintptr_t& ped, D3DXVECTOR3& pos) {
    ped = reinterpret_cast<uintptr_t>(SDK::Pointers::pLocalPlayer);
    float health = 0;
    return g_MenuInfo.IsLogged && Read(ped + 0x280, health) &&
        std::isfinite(health) && health > 100.f && Read(ped + 0x90, pos) && Sane(pos);
}
inline bool Forward(uintptr_t ped, D3DXVECTOR3& f) {
    if (!Read(ped + 0x70, f) || !Sane(f)) return false;
    f.z = 0;
    const float length = std::sqrt(f.x*f.x + f.y*f.y);
    if (!std::isfinite(length) || length < 0.1f || length > 2.f) return false;
    f.x /= length; f.y /= length;
    return true;
}
// Dicionario/anim de SEGURAR — ped levanta o braco com o carro acima
// "raise_the_roof" e uma celebracao do GTA Online base, presente em todos os servidores
constexpr const char* k_GrabDict = "anim@mp_player_intcelebrationmale@raise_the_roof";
constexpr const char* k_GrabAnim = "raise_the_roof_01_player";
// Dicionario/anim de ARREMESSAR — ped joga para frente
// "random@domestic" e um dict de NPCs domesticos, sempre disponivel no jogo base
constexpr const char* k_ThrowDict = "random@domestic";
constexpr const char* k_ThrowAnim = "domestic_v1_m";

inline void Release(const char* message = "Veiculo solto") {
    if (held) BridgeFeatures::StopAnimation();
    held = heldModel = heldPed = 0; heldPid = 0; status = message;
}
inline bool Move(uintptr_t vehicle, D3DXVECTOR3 destination) {
    uintptr_t nav = 0;
    D3DXVECTOR3 oldPos, oldNav, velocity;
    if (!Sane(destination) || !Listed(vehicle) ||
        !Read(vehicle + 0x30, nav) || nav < 0x10000 ||
        !Read(vehicle + 0x90, oldPos) || !Sane(oldPos) ||
        !Read(nav + 0x50, oldNav) || !Sane(oldNav) ||
        !Read(vehicle + 0x320, velocity) || !Sane(velocity)) return false;
    if (!Write(nav + 0x50, destination)) return false;
    if (!Write(vehicle + 0x90, destination)) {
        Write(nav + 0x50, oldNav); return false;
    }
    if (!Write(vehicle + 0x320, D3DXVECTOR3(0,0,0))) {
        Write(nav + 0x50, oldNav); Write(vehicle + 0x90, oldPos); return false;
    }
    return true;
}
inline const char* Pull(uintptr_t vehicle, int slot = 0) {
    uintptr_t ped = 0; D3DXVECTOR3 pos, f;
    if (!Player(ped, pos) || !Forward(ped, f) || !Listed(vehicle))
        return "Personagem ou veiculo indisponivel";
    if (SDK::Pointers::pLocalPlayer->InVehicle() &&
        reinterpret_cast<uintptr_t>(SDK::Pointers::pLocalPlayer->GetLastVehicle()) == vehicle)
        return "Saia do veiculo antes de puxa-lo";
    if (held == vehicle) Release();
    const float side = (slot % 2 ? -1.f : 1.f) * ((slot + 1) / 2) * 4.f;
    // Preserve clearance for multi-pull; the selected car lands close to the character.
    const D3DXVECTOR3 target(pos.x + f.x*3.f - f.y*side,
        pos.y + f.y*3.f + f.x*side, pos.z + 0.5f);
    auto reader=[](uintptr_t a,auto& v){return Read(a,v);};
    auto writer=[](uintptr_t a,const auto& v){return Write(a,v);};
    const bool applied=XrcVehiclePull::Apply(vehicle,ped,target,reader,writer,[](){
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    });
    return applied ? "XRC: posicao confirmada neste instante; sincronizacao ainda nao validada"
                   : "XRC: falha ao aplicar ou confirmar movimento do carro";
}
inline void PullFive() {
    std::vector<uintptr_t> targets;
    {
        std::lock_guard<std::mutex> lock(Core::Threads::g_VehicleList.vehicleListMutex);
        for (const auto& v : SDK::Game::VehicleList) {
            if (std::isfinite(v.Dist) && v.Dist <= 200.f && v.Pointer)
                targets.push_back(reinterpret_cast<uintptr_t>(v.Pointer));
            if (targets.size() == 5) break;
        }
    }
    for (size_t i = 0; i < targets.size(); ++i) Pull(targets[i], static_cast<int>(i));
    status = targets.empty() ? "Nenhum carro carregado a ate 200 m" : "Puxar carros solicitado";
}
inline bool Grab(uintptr_t vehicle) {
    uintptr_t ped = 0, model = 0; D3DXVECTOR3 pos, vehiclePos;
    if (!Player(ped, pos) || SDK::Pointers::pLocalPlayer->InVehicle() || !Listed(vehicle) ||
        !Read(vehicle + 0x20, model) || model < 0x10000 ||
        !Read(vehicle + 0x90, vehiclePos) || !Sane(vehiclePos)) {
        status = "Saia do carro e selecione um veiculo carregado"; return false;
    }
    const auto d = vehiclePos - pos;
    if (d.x*d.x+d.y*d.y+d.z*d.z > 200.f*200.f) {
        status = "Veiculo distante: limite de 200 m"; return false;
    }
    held = vehicle; heldModel = model; heldPed = ped; heldPid = Mem.ProcId;
    enabled = true; status = "Agarrado: Y arremessa; X solta; setas ajustam altura";
    // Animar ped: bracos levantados enquanto segura
    BridgeFeatures::Animate(k_GrabDict, k_GrabAnim, true);
    return true;
}
inline bool HeldValid(uintptr_t ped) {
    uintptr_t model = 0;
    return held && heldPid == Mem.ProcId && heldPed == ped && Listed(held) &&
        Read(held + 0x20, model) && model == heldModel;
}
inline void Throw() {
    uintptr_t ped = 0; D3DXVECTOR3 pos, f;
    if (!Player(ped, pos) || !HeldValid(ped) || !Forward(ped, f)) {
        Release("Veiculo indisponivel para arremessar"); return;
    }
    const float speed = ImClamp(throwSpeed, 10.f, 100.f);
    const bool ok = Write(held + 0x320, D3DXVECTOR3(f.x*speed, f.y*speed, speed*0.2f));
    // Trocar para animacao de lancamento antes de liberar
    if (ok) BridgeFeatures::Animate(k_ThrowDict, k_ThrowAnim, false);
    Release(ok ? "Arremesso local solicitado" : "Falha ao arremessar");
}
inline uintptr_t AimedVehicle() {
    std::vector<SDK::Game::VehicleStructure> list;
    { std::lock_guard<std::mutex> lock(Core::Threads::g_VehicleList.vehicleListMutex); list = SDK::Game::VehicleList; }
    float best = 120.f; uintptr_t result = 0;
    for (const auto& v : list) {
        if (!v.Pointer || !Sane(v.Pos) || !std::isfinite(v.Dist) || v.Dist > 200.f) continue;
        const auto p = SDK::Game::WorldToScreen(v.Pos);
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || p.x <= 0 || p.y <= 0 ||
            p.x >= g_Variables.g_vGameWindowSize.x || p.y >= g_Variables.g_vGameWindowSize.y) continue;
        const float d = std::hypot(p.x-g_Variables.g_vGameWindowCenter.x, p.y-g_Variables.g_vGameWindowCenter.y);
        if (d < best) { best = d; result = reinterpret_cast<uintptr_t>(v.Pointer); }
    }
    return result;
}
inline void Tick(bool stop = false) {
    uintptr_t ped = 0; D3DXVECTOR3 pos;
    if (stop || !enabled || !Player(ped, pos)) {
        Release(enabled && !stop ? "Personagem indisponivel" : "Desativado"); keyLatch = true; return;
    }
    DWORD foregroundPid=0; GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
    if (GetForegroundWindow() != g_Variables.g_hGameWindow &&
        !(g_MenuInfo.IsOpen && foregroundPid==GetCurrentProcessId())) {
        Release("Solto ao sair da janela do jogo"); keyLatch = true; return;
    }
    if (held && (!HeldValid(ped) || SDK::Pointers::pLocalPlayer->InVehicle())) {
        Release("Veiculo ou personagem mudou");
    }
    const bool menuOpen=g_MenuInfo.IsOpen;
    if(menuOpen) keyLatch=true;
    const bool key = !menuOpen && (GetAsyncKeyState('Y') & 0x8000) != 0;
    if (!menuOpen && (GetAsyncKeyState('X') & 0x8000)) Release();
    else if (key && !keyLatch) {
        if (held) Throw();
        else if (!Grab(AimedVehicle())) status = "Nenhum carro valido na mira a ate 200 m";
    }
    if(!menuOpen) keyLatch = key;
    if (!held) return;
    const float dt = ImClamp(ImGui::GetIO().DeltaTime, 0.f, 0.05f);
    if (!menuOpen && (GetAsyncKeyState(VK_UP) & 0x8000)) height += dt*6.f;
    if (!menuOpen && (GetAsyncKeyState(VK_DOWN) & 0x8000)) height -= dt*6.f;
    height = ImClamp(height, 3.f, 30.f);
    D3DXVECTOR3 current;
    if (!Read(held + 0x90, current) || !Sane(current)) { Release("Leitura do carro falhou"); return; }
    const D3DXVECTOR3 target(pos.x, pos.y, pos.z + height);
    const float blend = 1.f - std::exp(-12.f*dt);
    if (!Move(held, current + (target-current)*blend)) Release("Movimentacao do carro falhou");
}
}}}
