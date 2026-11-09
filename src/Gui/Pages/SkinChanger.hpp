#pragma once
#include <Includes/Includes.hpp>
#include <Core/Features/SkinChanger.hpp>

namespace SkinChangerPage {

    inline void Render() {
        using namespace Core::Features;

        uint32_t curHash = g_SkinChanger.GetCurrentWeaponHash();
        int32_t  curTint = g_SkinChanger.GetCurrentTint();

        ImGui::SetCursorPos(ImVec2(184, 76));
        ImGui::BeginGroup();

        ImGui::TextColored(ImColor(g_Col.FeaturesText), xorstr("Arma equipada:"));
        ImGui::SameLine();
        {
            char buf[16];
            sprintf_s(buf, "0x%08X", curHash);
            ImGui::TextColored(ImColor(g_Col.SecundaryText), buf);
        }

        ImGui::TextColored(ImColor(g_Col.FeaturesText), xorstr("Tint atual:"));
        ImGui::SameLine();
        ImGui::TextColored(ImColor(g_Col.SecundaryText), std::to_string(curTint).c_str());

        ImGui::Separator();

        auto indices = g_SkinChanger.GetSkinsForCurrentWeapon();

        if (indices.empty()) {
            ImGui::TextColored(ImColor(g_Col.SecundaryText),
                xorstr("Equipe uma arma suportada."));
        } else {
            ImGui::BeginChild(xorstr("SkinList"), ImVec2(460, 340), false, 0);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 8));

            for (int idx : indices) {
                const auto& entry = g_SkinList[idx];
                bool selected = (g_SkinChanger.m_selectedSkin == idx);

                if (ImGui::ListSelectable(entry.displayName, &selected)) {
                    g_SkinChanger.m_selectedSkin = idx;
                    g_SkinChanger.ApplyTint(entry.tintIndex);
                    std::thread([]() {
                        NotifyManager::Send(xorstr("Skin aplicada!"), 2000);
                    }).detach();
                }
            }

            ImGui::PopStyleVar();
            ImGui::EndChild();
        }

        ImGui::EndGroup();
    }

} // SkinChangerPage
