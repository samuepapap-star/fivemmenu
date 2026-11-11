#pragma once
#include <Gui/Pages/Extras.hpp>
#include <Bridge/Pages.hpp>
namespace Categories {
inline int carTab = 0, playerTab = 0, weaponTab = 0, teleportTab = 0, selfTab=0, configTab=0;
inline void Tab(const char* name, int value, int& current, bool sameLine) {
    if (sameLine) ImGui::SameLine();
    if (Custom::SubTab(name, current == value)) current = value;
}
inline void Cars() {
    Tab("Geral",0,carTab,false); Tab("Lista",1,carTab,true);
    Tab("ESP",2,carTab,true); Tab("Enviar ao jogador",3,carTab,true);
    Tab("Extras",4,carTab,true);
    if (carTab == 0) Local::Render(Local::iSubTabCount == 4 ? 4 : 1);
    else if (carTab == 1) World::Render(1);
    else if (carTab == 2) Visuals::Render(1);
    else if(carTab==3) World::Render(0,1);
    else ExtraPages::Render(1);
}
inline void Players() {
    Tab("ESP",0,playerTab,false); Tab("Radar e extras",1,playerTab,true);
    if (playerTab == 0) Visuals::Render(0); else ExtraPages::Render(3);
}
inline void WeaponSkinchanger() {
    ImGui::SetCursorPos(ImVec2(184,136));
    ImGui::BeginChild("Skinchanger##Weapons",ImVec2(704,440),false,0);
    ImGui::TextDisabled("Armas / Skinchanger");
    ImGui::TextWrapped("Equipe a arma que deseja personalizar antes de aplicar.");
    static int tint = 0;
    ImGui::SetNextItemWidth(320);
    ImGui::Combo("Cor da arma", &tint, "Preto (0)\0Verde (1)\0Dourado (2)\0Rosa (3)\0Exercito (4)\0Policia (5)\0Laranja (6)\0Platina (7)\0");
    ImGui::TextWrapped("Cores nativas de 0 a 7. A disponibilidade e a aparencia dependem da arma; modelos e texturas personalizados nao sao carregados aqui.");
    const bool busy = MenuBridge::Connection().Busy();
    if (!busy) {
        if (Custom::Button("Aplicar na arma equipada",ImVec2(320,33),0))
            BridgeFeatures::ApplyWeaponSkin(tint);
        if (Custom::Button("Aplicar cor padrao (0)",ImVec2(320,33),0)) {
            tint = 0;
            BridgeFeatures::ApplyWeaponSkin(tint);
        }
    } else {
        ImGui::TextDisabled("Aguarde a acao em andamento.");
    }
    ImGui::Separator();
    BridgePages::Footer();
    ImGui::EndChild();
}
inline void Weapons() {
    Tab("Ajustes",0,weaponTab,false); Tab("Catálogo",1,weaponTab,true); Tab("Extras",2,weaponTab,true); Tab("Skinchanger",3,weaponTab,true);
    if (weaponTab == 0) Local::Render(3); else if(weaponTab==1) Exploits::Render(0); else if(weaponTab==2) ExtraPages::Render(2); else WeaponSkinchanger();
}
inline void Teleports() {
    Tab("Destinos",0,teleportTab,false); Tab("Jogadores",1,teleportTab,true); Tab("Posicao salva",2,teleportTab,true);
    if (teleportTab == 0) Local::Render(2);
    else if(teleportTab==1) World::Render(0,2); else ExtraPages::Render(4);
}
inline void Personagem(){Tab("Geral",0,selfTab,false);Tab("Extras",1,selfTab,true);Tab("Modelos",2,selfTab,true);if(!selfTab)Local::Render(0);else if(selfTab==1)ExtraPages::Render(0);else BridgePages::Render(0);}
inline void Settings(){Tab("Geral",0,configTab,false);Tab("Aparencia",1,configTab,true);Tab("Ambiente",2,configTab,true);if(!configTab)::Settings::Render();else ExtraPages::Render(configTab==1?5:7);}
}

