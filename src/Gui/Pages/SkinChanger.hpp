#pragma once
#include <cstdio>

namespace SkinChanger {
inline constexpr int page = 101;
inline void Render() {
    static const char* names[] = {"Angel Love / ASSAULTRIFLE ANGEL",
        "Angel Love / CARBINERIFLE ANGEL",
        "Angel Love / SPECIALCARBINE ANGEL",
        "Angel Love / BAT ANGEL",
        "Angel Love / DAGGER ANGEL",
        "Angel Love / COMBATPISTOL ANGEL",
        "Angel Love / PISTOLMK2 ANGEL",
        "Angel Love / ASSAULTSMG ANGEL",
        "Angel Love / COMPACTSMG ANGEL",
        "Angel Love / MICROSMG ANGEL",
        "Angel Love / SMG ANGEL",
        "PenhaShop / PL G3 DE AGUA",
        "PenhaShop / PL PENHASHOP AR15 NIC",
        "PenhaShop / PL PISTOLA AGUA",
        "PenhaShop / PL SMG VERAO PUDDIN",
        "Glock PenhaShop / GLOCK RAJADA ANIME",
        "Glock PenhaShop / GLOCK RAJADA FADE",
        "Glock PenhaShop / GLOCK RAJADA GRAFITE",
        "Glock PenhaShop / GLOCK RAJADA OURO",
        "ESQUELETO / ESQUELETO",
        "G3 / G3",
        "G36GOAT / G36GOAT",
        "ZOLP / BARRETMATA",
        "ZOLP / BDRX15",
        "ZOLP / BRS74",
        "ZOLP / FALGOLDNM",
        "ZOLP / GGOLDLP",
        "ZOLP / M249G"};
    static_assert(IM_ARRAYSIZE(names) == 28, "O catalogo deve preservar os IDs 0-27.");
    static int selected = 0;
    static const char* feedback = "Nenhum comando copiado; aplicacao nao confirmada.";
    // Categorias visuais; selected conserva o indice original do plugin.
    static const char* categories[] = {
        "Todas", "Glock", "G3", "Pistolas", "Fuzis", "SMGs",
        "Angel Love", "Especiais"
    };
    static int category = 0;
    static const int categoryByIndex[] = {
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        2, 4, 3, 5, 1, 1, 1, 1,
        7, 2, 4, 7, 4, 4, 4, 3, 7
    };
    static_assert(IM_ARRAYSIZE(categoryByIndex) == IM_ARRAYSIZE(names),
                  "Cada skin precisa de uma categoria.");
    ImGui::SetCursorPos(ImVec2(184,136));
    ImGui::BeginChild("Skin Changer##local",ImVec2(700,440),true);
    ImGui::TextUnformatted("Skin Changer");
    // Catalogo e selecao funcionam sem qualquer recurso FiveM ativo.
    if (ImGui::Combo("Categoria", &category, categories, IM_ARRAYSIZE(categories))) {
        for (int i = 0; i < IM_ARRAYSIZE(names); ++i) {
            if (category == 0 || categoryByIndex[i] == category) {
                selected = i;
                break;
            }
        }
    }
    const char* filteredNames[IM_ARRAYSIZE(names)]{};
    int filteredIndices[IM_ARRAYSIZE(names)]{};
    int filteredCount = 0;
    int filteredSelected = -1;
    for (int i = 0; i < IM_ARRAYSIZE(names); ++i) {
        if (category != 0 && categoryByIndex[i] != category) continue;
        filteredNames[filteredCount] = names[i];
        filteredIndices[filteredCount] = i;
        if (selected == i) filteredSelected = filteredCount;
        ++filteredCount;
    }
    int comboSelection = filteredSelected >= 0 ? filteredSelected : 0;
    if (filteredCount > 0 && ImGui::Combo("Modelo", &comboSelection, filteredNames, filteredCount))
        selected = filteredIndices[comboSelection];
    const bool canApply = category == 0 || categoryByIndex[selected] == category;
    ImGui::Text("Skin selecionada: %d / 27", selected);
    ImGui::TextWrapped("Aplicacao autorizada pelo servidor. O EXE prepara o comando; o resultado aparece no FiveM.");
    char command[64]{};
    std::snprintf(command, sizeof(command), "/skinlocal apply %d", selected);
    ImGui::TextUnformatted(command);
    ImGui::BeginDisabled(!canApply);
    if (ImGui::Button("Copiar comando Aplicar")) {
        ImGui::SetClipboardText(command);
        feedback = "Comando copiado. Aguarde o resultado no FiveM; nenhuma aplicacao confirmada aqui.";
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Copiar comando Restaurar")) {
        ImGui::SetClipboardText("/skinlocal restore");
        feedback = "Comando copiado. Restauracao ainda nao confirmada; consulte o FiveM.";
    }
    ImGui::TextWrapped("%s", feedback);
    ImGui::Separator();
    ImGui::TextWrapped("Cole no chat do FiveM. Requer SkinChangerLocal instalado pelo administrador e permissao command.skinlocal.");
    ImGui::TextWrapped("Copiar nao aplica a skin. Entradas sem mapeamento validado pelo administrador sao recusadas. Modelos add-on nao sao convertidos em componentes automaticamente.");
    ImGui::EndChild();
}
}
