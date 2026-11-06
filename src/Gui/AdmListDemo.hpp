#pragma once
#include <Includes/Includes.hpp>
#include <cstdlib>

namespace AdmListDemo {
inline double changedAt = 0.0, interval = 0.0;
inline bool visible = false;
inline void Draw() {
    const double now = ImGui::GetTime();
    if (!g_MenuInfo.IsLogged || !g_Config.ESP->Enabled || !g_Config.ESP->AdmListDemo) {
        changedAt = now; interval = 0.0; visible = false;
        return;
    }
    // Lipeee's timed visual animation, explicitly presented as a simulation.
    // Does not inspect entities, invisibility flags, roles or server permissions.
    if (now - changedAt >= interval) {
        visible = !visible;
        changedAt = now;
        interval = visible ? 4.0 : 10.0 + (std::rand() % 41);
    }
    if (!visible) return;
    const int distance = (std::min)(120, 30 + int((now - changedAt) * 10.0));
    const std::string heading = "ADM list - SIMULACAO";
    const std::string text = "Personagem invisivel (simulado): " + std::to_string(distance) + " m";
    const char* note = "Exemplo visual, sem deteccao de administradores.";
    const float width = (std::max)(ImGui::CalcTextSize(text.c_str()).x, ImGui::CalcTextSize(note).x) + 24.f;
    const float line = ImGui::GetTextLineHeightWithSpacing();
    const float height = line * 3.f + 20.f;
    const ImVec2 screen = g_Variables.g_vGameWindowSize;
    const ImVec2 pos((std::max)(0.f, (screen.x - width) / 1.65f),
        (std::max)(0.f, (screen.y - height) * 0.5f));
    auto draw = ImGui::GetBackgroundDrawList();
    draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(20,20,20,235), 6.f);
    draw->AddText(ImVec2(pos.x + 12, pos.y + 10), IM_COL32(255,190,60,255), heading.c_str());
    draw->AddText(ImVec2(pos.x + 12, pos.y + 10 + line), IM_COL32(240,240,240,255), text.c_str());
    draw->AddText(ImVec2(pos.x + 12, pos.y + 10 + line * 2), IM_COL32(170,170,170,255), note);
}
}
