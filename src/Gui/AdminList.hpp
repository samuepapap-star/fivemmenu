#pragma once
#include <Core/ExtensionSettings.hpp>
#include <Core/SDK/SDK.hpp>
#include <Core/Features/PlayerBones.hpp>
#include <cmath>
#include "VisibilityEvidence.hpp"
namespace AdminList {
inline std::vector<std::string> entries;
template<class T> inline bool Read(uintptr_t address,T& value) {
    SIZE_T bytes=0;
    return address && Mem.ProcHandle && ReadProcessMemory(Mem.ProcHandle,
        reinterpret_cast<LPCVOID>(address),&value,sizeof(value),&bytes) && bytes==sizeof(value);
}

// RGB rainbow color cycle keyed by time — same concept as gouchuparola adminEspNames RGB
inline ImColor RgbCycleColor(float speed=1.f) {
    const float t = static_cast<float>(GetTickCount64() % 3000) / 3000.f * speed;
    const float h = fmodf(t, 1.f);
    // HSV→RGB, S=1, V=1
    const float s=1.f, v=1.f;
    const int i = static_cast<int>(h*6.f);
    const float f=h*6.f-i, p=v*(1-s), q=v*(1-s*f), tv=v*(1-s*(1-f));
    float r,g,b;
    switch(i%6){
        case 0:r=v;g=tv;b=p;break; case 1:r=q;g=v;b=p;break;
        case 2:r=p;g=v;b=tv;break; case 3:r=p;g=q;b=v;break;
        case 4:r=tv;g=p;b=v;break; default:r=v;g=p;b=q;break;
    }
    return ImColor(r,g,b,1.f);
}

// Skeleton draw reused locally (mirrors Esp.cpp anonymous DrawTracelessSkeleton)
inline void DrawAdminSkeleton(CPed* ped, ImDrawList* draw, ImColor color) {
    Core::Features::PlayerBones bones;
    if (!bones.Open(ped)) return;
    enum Joint { Pelvis,Neck,LC,RC,LA,RA,LF,RF,LH,RH,LT,RT,LCalf,RCalf,LFoot,RFoot,JointCount };
    constexpr unsigned masks[JointCount]={0x2e28,0x9995,0xfcd9,0x29d2,0xb1c5,0x9d4d,
        0xeeeb,0x6e5c,0x49d9,0xdead,0xe39f,0xca72,0xf9bb,0x9000,0x3779,0xcc4d};
    constexpr Joint links[][2]={{Pelvis,Neck},{Neck,LC},{Neck,RC},{LC,LA},{RC,RA},{LA,LF},
        {RA,RF},{LF,LH},{RF,RH},{Pelvis,LT},{Pelvis,RT},{LT,LCalf},{RT,RCalf},
        {LCalf,LFoot},{RCalf,RFoot}};
    ImVec2 pts[JointCount]{};
    bool vis[JointCount]{};
    for(int i=0;i<JointCount;++i){
        D3DXVECTOR3 bone;
        if(!bones.Position(masks[i],bone))continue;
        const auto s=Core::SDK::Game::WorldToScreen(bone);
        const auto scrSz=ImGui::GetIO().DisplaySize;
        vis[i]=(s.x>=0&&s.y>=0&&s.x<=scrSz.x&&s.y<=scrSz.y);
        pts[i]=ImVec2(s.x,s.y);
    }
    for(const auto& lk:links)
        if(vis[lk[0]]&&vis[lk[1]])
            draw->AddLine(pts[lk[0]],pts[lk[1]],color,ExtraSettings::SkeletonWidth);
}

inline void Draw() {
    const auto cfg=g_Config.ESP;
    entries.clear();
    static VisibilityEvidence::Filter filter;
    static DWORD processId=0;
    if(processId!=Mem.ProcId){filter.Clear();processId=Mem.ProcId;}

    const bool anyAdminEsp = cfg->AdminEspBox||cfg->AdminEspSnapLines||cfg->AdminEspSkeleton;
    if (!g_MenuInfo.IsLogged || (!cfg->Enabled && !ExtraSettings::InvisibleWarning) ||
        (!cfg->AdminList && !cfg->AdminHighlight && !anyAdminEsp && !ExtraSettings::InvisibleWarning)) {
        filter.Clear();
        return;
    }
    const auto now=GetTickCount64();
    filter.Prune(now);
    auto& rows=entries;
    auto draw=ImGui::GetBackgroundDrawList();
    const auto scrSz=ImGui::GetIO().DisplaySize;
    const ImVec2 screenCenter(scrSz.x*0.5f, scrSz.y);

    for (const auto& entity:Core::SDK::Game::EntityList) {
        if (!entity.Ped || entity.Ped==Core::SDK::Pointers::pLocalPlayer || entity.PedType!=2 ||
            entity.Health<=101.f || entity.Distance>cfg->MaxDistance || !std::isfinite(entity.Distance)) continue;
        const auto pos=entity.Pos;
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z) ||
            (pos.x==0 && pos.y==0 && pos.z==0)) continue;
        BYTE visibility=0,alpha=255;
        const uintptr_t ped=reinterpret_cast<uintptr_t>(entity.Ped);
        if (!Read(ped+0x2C,visibility) || !Read(ped+0xAC,alpha)) continue;
        if (!filter.Observe(ped,entity.Id,visibility,alpha,entity.Ped->InVehicle(),now)) continue;
        // HyperX heuristic: visibility flags only. No administrator role information.
        const std::string name=entity.NetworkInfo.UserName.empty()?"Desconhecido":entity.NetworkInfo.UserName;
        rows.push_back(name+" | ID "+std::to_string(entity.Id)+" | "+std::to_string(int(entity.Distance))+" m");

        const auto screenFeet=Core::SDK::Game::WorldToScreen(pos);
        const bool onScreen=Core::SDK::Game::IsOnScreen(screenFeet);

        // Legacy text marker
        if (cfg->AdminHighlight && cfg->Enabled && onScreen)
            draw->AddText(ImGui::GetFont(),11.f,ImVec2(screenFeet.x,screenFeet.y),IM_COL32(255,255,255,255),"ADM");

        // --- Admin ESP visual overlay (box, snap lines, RGB names, skeleton) ---
        if (!cfg->Enabled || !onScreen) continue;

        // Estimate head position: 1.8m above feet
        D3DXVECTOR3 headPos=pos; headPos.z+=1.8f;
        const auto screenHead=Core::SDK::Game::WorldToScreen(headPos);
        if (!Core::SDK::Game::IsOnScreen(screenHead)) continue;

        const float boxH=std::abs(screenFeet.y-screenHead.y);
        const float boxW=boxH*0.4f;
        const ImVec2 boxMin(screenHead.x-boxW*0.5f, screenHead.y);
        const ImVec2 boxMax(screenHead.x+boxW*0.5f, screenFeet.y);
        const ImColor adminCol=cfg->AdminEspCol;

        if (cfg->AdminEspBox) {
            draw->AddRect(boxMin, boxMax, adminCol, 0.f, 0, 1.5f);
        }
        if (cfg->AdminEspSnapLines) {
            draw->AddLine(screenCenter, ImVec2(screenFeet.x,screenFeet.y), adminCol, 1.f);
        }
        // RGB names sempre ativos para ADMs detectados
        {
            const ImColor rgbCol=RgbCycleColor();
            const ImVec2 namePos(screenHead.x, screenHead.y-14.f);
            draw->AddText(ImGui::GetFont(),13.f,ImVec2(namePos.x+1.f,namePos.y+1.f),IM_COL32(0,0,0,200),name.c_str());
            draw->AddText(ImGui::GetFont(),13.f,namePos,rgbCol,name.c_str());
        }
        if (cfg->AdminEspSkeleton) {
            DrawAdminSkeleton(entity.Ped, draw, adminCol);
        }
    }
    // The list is rendered inside the menu; no background panel covers gameplay.
}
inline void RenderEntries() {
    if(entries.empty())ImGui::TextWrapped("Nenhum indicador nos jogadores carregados.");
    for(const auto& row:entries)ImGui::TextWrapped("%s",row.c_str());
}
}

