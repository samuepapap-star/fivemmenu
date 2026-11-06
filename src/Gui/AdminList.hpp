#pragma once
#include <Core/SDK/SDK.hpp>
#include <cmath>
namespace AdminList {
inline std::vector<std::string> entries;
template<class T> inline bool Read(uintptr_t address,T& value) {
    SIZE_T bytes=0;
    return address && Mem.ProcHandle && ReadProcessMemory(Mem.ProcHandle,
        reinterpret_cast<LPCVOID>(address),&value,sizeof(value),&bytes) && bytes==sizeof(value);
}
inline void Draw() {
    const auto cfg=g_Config.ESP;
    entries.clear();
    if (!g_MenuInfo.IsLogged || !cfg->Enabled || (!cfg->AdminList && !cfg->AdminHighlight)) return;
    auto& rows=entries;
    auto draw=ImGui::GetBackgroundDrawList();
    for (const auto& entity:Core::SDK::Game::EntityList) {
        if (!entity.Ped || entity.Ped==Core::SDK::Pointers::pLocalPlayer || entity.PedType!=2 ||
            entity.Health<=101.f || entity.Distance>cfg->MaxDistance || !std::isfinite(entity.Distance)) continue;
        const auto pos=entity.Pos;
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z) ||
            (pos.x==0 && pos.y==0 && pos.z==0)) continue;
        BYTE visibility=0,alpha=255;
        const uintptr_t ped=reinterpret_cast<uintptr_t>(entity.Ped);
        if (!Read(ped+0x2C,visibility) || !Read(ped+0xAC,alpha)) continue;
        if ((visibility&1)!=0 && alpha!=0) continue;
        // HyperX heuristic: visibility flags only. No administrator role information.
        const std::string name=entity.NetworkInfo.UserName.empty()?"Desconhecido":entity.NetworkInfo.UserName;
        rows.push_back(name+" | ID "+std::to_string(entity.Id)+" | "+std::to_string(int(entity.Distance))+" m");
        if (cfg->AdminHighlight) {
            const auto screen=Core::SDK::Game::WorldToScreen(pos);
            if (Core::SDK::Game::IsOnScreen(screen))
                draw->AddText(ImVec2(screen.x,screen.y),IM_COL32(255,190,60,255),"Possivel invisivel");
        }
    }
    // The list is rendered inside the menu; no background panel covers gameplay.
}
inline void RenderEntries() {
    if(entries.empty())ImGui::TextWrapped("Nenhum indicador nos jogadores carregados.");
    for(const auto& row:entries)ImGui::TextWrapped("%s",row.c_str());
}
}

