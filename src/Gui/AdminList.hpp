#pragma once
#include <Core/SDK/SDK.hpp>
#include <cmath>
namespace AdminList {
template<class T> inline bool Read(uintptr_t address,T& value) {
    SIZE_T bytes=0;
    return address && Mem.ProcHandle && ReadProcessMemory(Mem.ProcHandle,
        reinterpret_cast<LPCVOID>(address),&value,sizeof(value),&bytes) && bytes==sizeof(value);
}
inline void Draw() {
    const auto cfg=g_Config.ESP;
    if (!g_MenuInfo.IsLogged || !cfg->Enabled || (!cfg->AdminList && !cfg->AdminHighlight)) return;
    std::vector<std::string> rows;
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
    if (!cfg->AdminList) return;
    const char* title="ADM list - possiveis invisiveis";
    const char* note="Indicadores locais; nao confirma cargo de administrador.";
    float width=(std::max)(ImGui::CalcTextSize(title).x,ImGui::CalcTextSize(note).x)+20.f;
    const size_t count=(std::min)(rows.size(),size_t(12));
    for(size_t i=0;i<count;++i)width=(std::max)(width,ImGui::CalcTextSize(rows[i].c_str()).x+20.f);
    const float line=ImGui::GetTextLineHeightWithSpacing();
    const float height=(float(count?count:1)+2.f)*line+20.f;
    const float x=(std::max)(0.f,g_Variables.g_vGameWindowSize.x-width-15.f),y=65.f;
    draw->AddRectFilled(ImVec2(x,y),ImVec2(x+width,y+height),IM_COL32(20,20,20,235),6.f);
    draw->AddText(ImVec2(x+10,y+10),IM_COL32(255,190,60,255),title);
    if(rows.empty())draw->AddText(ImVec2(x+10,y+10+line),IM_COL32(210,210,210,255),"Nenhum indicador encontrado nos jogadores carregados.");
    for(size_t i=0;i<count;++i)draw->AddText(ImVec2(x+10,y+10+line*(i+1)),IM_COL32(240,240,240,255),rows[i].c_str());
    draw->AddText(ImVec2(x+10,y+height-line-6),IM_COL32(170,170,170,255),note);
}
}
