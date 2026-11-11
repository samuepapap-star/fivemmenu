#pragma once
#include <Bridge/Features.hpp>
#include <Bridge/Pages.hpp>

namespace SkinChanger {
inline constexpr int page = 101;
inline void Render() {
    ImGui::SetCursorPos(ImVec2(184,136));
    ImGui::BeginChild("Skin Changer##local",ImVec2(700,440),true);
    ImGui::TextUnformatted("Skin de Arma (somente voce ve)");
    ImGui::TextWrapped("Muda a cor/acabamento da arma que voce esta segurando. Visivel apenas para voce, sem sincronizacao em rede.");
    ImGui::Separator();

    static const char* tintNames[] = {
        "Normal (padrao)",
        "Verde",
        "Dourado",
        "Cor de Rosa",
        "Exercito",
        "LSPD",
        "Laranja",
        "Platina"
    };
    static int selected = 0;
    ImGui::TextUnformatted("Selecione o acabamento:");
    for (int i = 0; i < IM_ARRAYSIZE(tintNames); ++i) {
        if (ImGui::RadioButton(tintNames[i], selected == i))
            selected = i;
    }
    ImGui::Separator();
    if (Custom::Button("Aplicar na arma atual", ImVec2(-1, 36), 0))
        BridgeFeatures::ApplyWeaponSkin(selected);
    ImGui::TextWrapped("Segure a arma que deseja mudar antes de clicar em Aplicar.");
    BridgePages::Footer();
    ImGui::EndChild();
}
}
