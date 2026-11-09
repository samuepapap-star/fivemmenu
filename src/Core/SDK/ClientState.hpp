#pragma once
#include <Core/SDK/SDK.hpp>
#include <Core/SDK/TeleportTrace.hpp>
#include <sstream>
#include <chrono>
namespace ClientState {
struct Snapshot {
    DWORD windowPid=0,attachedPid=0,error=0;
    int build=0,pedCapacity=0;
    bool processAlive=false,world=false,player=false,replay=false,pedList=false,viewport=false,camera=false;
    std::string status="Aguardando diagnóstico";
};
inline Snapshot current;
template<class T> inline bool Read(uintptr_t address,T& value,DWORD& error) {
    SIZE_T bytes=0;
    if(!address || !Mem.ProcHandle) {error=ERROR_INVALID_ADDRESS;return false;}
    if(!ReadProcessMemory(Mem.ProcHandle,reinterpret_cast<LPCVOID>(address),&value,sizeof(value),&bytes)) {
        error=GetLastError();return false;
    }
    if(bytes!=sizeof(value)) {error=ERROR_PARTIAL_COPY;return false;}
    return true;
}
inline void Tick() {
    using Clock=std::chrono::steady_clock;
    static Clock::time_point next{};
    if(Clock::now()<next)return;
    next=Clock::now()+std::chrono::seconds(1);
    Snapshot s;
    s.build=g_Offsets.CurrentBuild;
    s.attachedPid=Mem.ProcId;
    const HWND window=FindWindowA("grcWindow",nullptr);
    if(window)GetWindowThreadProcessId(window,&s.windowPid);
    DWORD exitCode=0;
    s.processAlive=Mem.ProcHandle && GetExitCodeProcess(Mem.ProcHandle,&exitCode) && exitCode==STILL_ACTIVE;
    uintptr_t world=0,ped=0,replay=0,pedInterface=0,list=0,viewport=0,camera=0;
    s.world=Read(g_Offsets.m_World,world,s.error) && world;
    s.player=s.world && Read(world+8,ped,s.error) && ped;
    s.replay=Read(g_Offsets.m_ReplayInterFace,replay,s.error) && replay;
    s.pedList=s.replay && Read(replay+0x18,pedInterface,s.error) && pedInterface &&
        Read(pedInterface+0x100,list,s.error) && list && Read(pedInterface+0x108,s.pedCapacity,s.error) &&
        s.pedCapacity>0 && s.pedCapacity<=4096;
    s.viewport=Read(g_Offsets.m_ViewPort,viewport,s.error) && viewport;
    s.camera=Read(g_Offsets.m_CamGameplayDirector,camera,s.error) && camera;
    if(!s.processAlive)s.status="Processo indisponível: reinicie o EXE com o FiveM aberto.";
    else if(!s.windowPid || s.attachedPid!=s.windowPid)s.status="A janela e o processo associado não correspondem. Reinicie o EXE.";
    else if(!s.world || !s.player)s.status="Personagem indisponível: carregamento ou dados incompatíveis.";
    else if(!s.pedList)s.status="Lista de personagens indisponível.";
    else if(!s.viewport)s.status="Dados de projeção da ESP indisponíveis.";
    else if(!s.camera)s.status="Dados da câmera indisponíveis.";
    else s.status="Leituras básicas do jogo disponíveis.";
    std::ostringstream log;
    log<<"client_state attached_pid="<<s.attachedPid<<" window_pid="<<s.windowPid<<" build="<<s.build
       <<" alive="<<s.processAlive<<" world="<<s.world<<" player="<<s.player<<" replay="<<s.replay
       <<" ped_list="<<s.pedList<<" ped_capacity="<<s.pedCapacity<<" viewport="<<s.viewport
       <<" camera="<<s.camera<<" read_error="<<s.error;
    static std::string previous;
    if(previous!=log.str()) {previous=log.str();AltosDiagnostics::Log(previous);}
    current=s;
}
inline void Render() {
    Tick();
    ImGui::Separator();
    ImGui::TextUnformatted("Diagnóstico do FiveM");
    ImGui::Text("Build: %d | PID: %lu",current.build,static_cast<unsigned long>(current.attachedPid));
    ImGui::TextWrapped("%s",current.status.c_str());
    ImGui::Text("Mundo: %s | Personagem: %s",current.world?"OK":"Falhou",current.player?"OK":"Falhou");
    ImGui::Text("Lista: %s | Projeção: %s",current.pedList?"OK":"Falhou",current.viewport?"OK":"Falhou");
    ImGui::TextWrapped("Registro: %%TEMP%%/ALTOS_PODERES_teleporte.log");
}
}
