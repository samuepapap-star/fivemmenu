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
        draw->AddRectFilled(pos,pos+size,ImGui::GetColorU32(g_Col.BackgroundCol),6.f);
        draw->AddRectFilled(pos,pos+ImVec2(160,size.y),ImGui::GetColorU32(g_Col.SideBar),6.f,ImDrawFlags_RoundCornersLeft);
        draw->AddRect(pos,pos+size,ImGui::GetColorU32(g_Col.BorderCol),6.f);
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
        auto section=[&](const char* label) {
            ImGui::SetCursorPos(ImVec2(12,y));
            ImGui::TextColored(ImColor(120,120,120),"%s",label);
            y+=23.f;
        };
        auto item=[&](const char* icon,const char* label,int page,int combat=-1) {
            if(search[0]) {
                std::string text(label),query(search);
                std::transform(text.begin(),text.end(),text.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
                std::transform(query.begin(),query.end(),query.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
                if(text.find(query)==std::string::npos)return;
            }
            ImGui::SetCursorPos(ImVec2(5,y));
            const bool active=g_MenuInfo.iTabCount==page && (combat<0 || Combat::iSubTabCount==combat);
            if(Custom::Tab(icon,label,active)) {g_MenuInfo.iTabCount=page;if(combat>=0)Combat::iSubTabCount=combat;}
            y+=34.f;
        };
        section("Combate");
        item(ICON_FA_CROSSHAIRS,"Mira assistida",g_MenuInfo.Combat,0);
        item(ICON_FA_BOLT,"Tiro automático",g_MenuInfo.Combat,1);
        item(ICON_FA_PERSON_RUNNING,"Mira silenciosa",g_MenuInfo.Combat,2);
        section("Recursos");
        item(ICON_FA_PERSON,"Personagem",g_MenuInfo.Local);
        item(ICON_FA_LIST,"Listas",g_MenuInfo.World);
        item(ICON_FA_BOMB,"Extras",g_MenuInfo.Exploits);
        section("Visual");
        item(ICON_FA_EARTH_AMERICAS,"ESP",g_MenuInfo.Visuals);
        section("Personalização");
        item(ICON_FA_GEAR,"Configurações",g_MenuInfo.Settings);
    }
}
