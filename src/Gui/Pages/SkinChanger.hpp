#pragma once
#include <windows.h>
#include <cwchar>
#include <cstdlib>
#include <cstring>

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
    static int selected = 0;
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
    static wchar_t file[MAX_PATH]{};
    if (!file[0]) {
        wchar_t base[MAX_PATH]{};
        DWORD size=GetEnvironmentVariableW(L"LOCALAPPDATA",base,MAX_PATH);
        if(size && size<MAX_PATH-40) swprintf_s(file,L"%ls\\SkinChangerLocal\\client.ini",base);
    }
    static ULONGLONG nextPoll=0;
    static bool online=false;
    static char status[384]{};
    const auto now=GetTickCount64();
    if(now>=nextPoll && file[0]) {
        wchar_t value[200]{};
        GetPrivateProfileStringW(L"renderer",L"heartbeat",L"",value,200,file);
        const auto heartbeat=_wcstoui64(value,nullptr,10);
        online=heartbeat && heartbeat<=now && now-heartbeat<=6000;
        GetPrivateProfileStringW(L"renderer",L"status",L"",value,200,file);
        WideCharToMultiByte(CP_UTF8,0,value,-1,status,sizeof(status),nullptr,nullptr);
        nextPoll=now+500;
    }
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
    ImGui::TextUnformatted("Catalogo disponivel no EXE; aplicacao exige recurso autorizado ativo.");
    ImGui::BeginDisabled(!online || !canApply);
    bool apply=ImGui::Button("Aplicar na arma equipada");
    ImGui::SameLine();
    bool restore=ImGui::Button("Restaurar original");
    if(apply || restore) {
        wchar_t command[64]{};
        swprintf_s(command,L"%llu:%d",now,restore ? -1 : selected);
        if(!WritePrivateProfileStringW(L"renderer",L"command",command,file))
            strcpy_s(status,"Falha ao enviar a seleção ao plugin.");
    }
    ImGui::EndDisabled();
    if (!canApply)
        ImGui::TextWrapped("Selecione um modelo desta categoria para aplicar.");
    if(!online) {
        ImGui::Separator();
        ImGui::TextWrapped("Modo catalogo: nenhuma integracao FiveM autorizada respondeu.");
        ImGui::TextWrapped("Voce pode navegar e selecionar skins, mas Aplicar e Restaurar ficam indisponiveis. O EXE nao injeta modelos no FiveM nem altera FiveM Application Data.");
    }
    else if(status[0]) ImGui::TextWrapped("%s",status);
    ImGui::EndChild();
}
}
