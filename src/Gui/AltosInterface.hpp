#pragma once
#include <shellapi.h>
#include <algorithm>
#include <cctype>
namespace AltosInterface {
    inline char search[64] = {};
    inline void Background() {
        const ImVec2 pos = ImGui::GetWindowPos();
        const ImVec2 size = g_MenuInfo.MenuSize;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(pos, pos + size, ImGui::GetColorU32(ImVec4(.06f,.06f,.06f,1)), 12.f);
        draw->AddRect(pos, pos + size, ImGui::GetColorU32(ImVec4(.12f,.12f,.12f,1)), 12.f);
        draw->AddLine(pos + ImVec2(0,60), pos + ImVec2(size.x,60), ImGui::GetColorU32(ImVec4(.12f,.12f,.12f,1)));
        draw->AddLine(pos + ImVec2(160,60), pos + ImVec2(160,size.y), ImGui::GetColorU32(ImVec4(.12f,.12f,.12f,1)));
        draw->AddText(pos + ImVec2(18,22), ImGui::GetColorU32(g_Col.Base), "|||");
        const char* title = "ALTOS PODERES";
        const ImVec2 text = ImGui::CalcTextSize(title);
        draw->AddText(pos + ImVec2((size.x-text.x)*.5f,21), ImGui::GetColorU32(ImVec4(.9f,.9f,.9f,1)), title);
        ImGui::SetCursorPos(ImVec2(53,16));
        ImGui::SetNextItemWidth(200.f);
        ImGui::InputTextWithHint("##page_search","Buscar páginas...",search,sizeof(search));
        ImGui::SetCursorPos(ImVec2(size.x-36,18));
        if (ImGui::InvisibleButton("##AltosClose",ImVec2(22,22)) && g_MenuInfo.IsOpen) {
            g_MenuInfo.IsOpen=false;
            const LONG_PTR style=GetWindowLongPtr(g_Variables.g_hCheatWindow,GWL_EXSTYLE);
            SetWindowLongPtr(g_Variables.g_hCheatWindow,GWL_EXSTYLE,style | WS_EX_TRANSPARENT);
            if(GetForegroundWindow()==g_Variables.g_hCheatWindow)
                SetForegroundWindow(g_Variables.g_hGameWindow);
        }
        draw->AddText(pos+ImVec2(size.x-31,20),ImGui::GetColorU32(ImVec4(.5f,.5f,.5f,1)),"X");

        const ImVec2 footer=pos+ImVec2(10,size.y-88);
        draw->AddRectFilled(footer,pos+ImVec2(150,size.y-10),ImGui::GetColorU32(ImVec4(.09f,.09f,.09f,1)),8.f);
        const char* discord="https://discord.gg/altospoderesfivem";
        draw->AddText(ImGui::GetFont(),11.f,footer+ImVec2(8,13),ImGui::GetColorU32(g_Col.Base),discord,nullptr,124.f);
        ImGui::SetCursorPos(ImVec2(10,size.y-88));
        if (ImGui::InvisibleButton("##AltosDiscord",ImVec2(140,78)) && g_MenuInfo.IsOpen)
            ShellExecuteA(nullptr,"open",discord,nullptr,nullptr,SW_SHOWNORMAL);
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s",discord);
    }
    inline void Navigation() {
        float y=76.f;
        auto section=[&](const char* name) {
            ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowPos()+ImVec2(12,y),
                ImGui::GetColorU32(ImVec4(.5f,.5f,.5f,1)),name);
            y+=26.f;
        };
        auto item=[&](const char* name,int page,int combatTab=-1) {
            if(search[0]) {
                std::string label(name),query(search);
                std::transform(label.begin(),label.end(),label.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
                std::transform(query.begin(),query.end(),query.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
                if(label.find(query)==std::string::npos) return;
            }
            const bool selected=g_MenuInfo.iTabCount==page &&
                (combatTab<0 || Combat::iSubTabCount==combatTab);
            ImGui::SetCursorPos(ImVec2(10,y));
            const bool clicked=ImGui::InvisibleButton(name,ImVec2(140,30));
            const ImVec2 p=ImGui::GetItemRectMin();
            auto* draw=ImGui::GetWindowDrawList();
            if(selected || ImGui::IsItemHovered())
                draw->AddRectFilled(p,p+ImVec2(140,30),ImGui::GetColorU32(ImVec4(.09f,.09f,.09f,1)),6.f);
            draw->AddCircleFilled(p+ImVec2(13,15),3.f,ImGui::GetColorU32(selected?g_Col.Base:ImVec4(.5f,.5f,.5f,1)));
            draw->AddText(p+ImVec2(26,7),ImGui::GetColorU32(selected?ImVec4(.9f,.9f,.9f,1):ImVec4(.5f,.5f,.5f,1)),name);
            if(clicked) {g_MenuInfo.iTabCount=page;if(combatTab>=0)Combat::iSubTabCount=combatTab;}
            y+=33.f;
        };
        section("Combate");
        item("Mira assistida",g_MenuInfo.Combat,0);
        item("Tiro automático",g_MenuInfo.Combat,1);
        item("Mira silenciosa",g_MenuInfo.Combat,2);
        y+=8.f;section("Visual");
        item("Jogadores",g_MenuInfo.Visuals);
        item("Mundo",g_MenuInfo.World);
        item("Personagem",g_MenuInfo.Local);
        item("Recursos extras",g_MenuInfo.Exploits);
        y+=8.f;section("Outros");
        item("Configurações",g_MenuInfo.Settings);
    }
}
