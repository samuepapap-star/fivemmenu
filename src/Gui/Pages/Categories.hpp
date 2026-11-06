#pragma once
namespace Categories {
inline int carTab = 0, playerTab = 0, weaponTab = 0, teleportTab = 0;
inline void Tab(const char* name, int value, int& current, bool sameLine) {
    if (sameLine) ImGui::SameLine();
    if (Custom::SubTab(name, current == value)) current = value;
}
inline void Cars() {
    Tab("Geral",0,carTab,false); Tab("Lista",1,carTab,true);
    Tab("ESP",2,carTab,true); Tab("Enviar ao jogador",3,carTab,true);
    if (carTab == 0) Local::Render(Local::iSubTabCount == 4 ? 4 : 1);
    else if (carTab == 1) World::Render(1);
    else if (carTab == 2) Visuals::Render(1);
    else World::Render(0,1);
}
inline void Players() {
    Tab("ESP",0,playerTab,false); Tab("Lista e ações",1,playerTab,true);
    if (playerTab == 0) Visuals::Render(0); else World::Render(0);
}
inline void Weapons() {
    Tab("Ajustes",0,weaponTab,false); Tab("Catálogo",1,weaponTab,true);
    if (weaponTab == 0) Local::Render(3); else Exploits::Render(0);
}
inline void Teleports() {
    Tab("Destinos",0,teleportTab,false); Tab("Jogadores",1,teleportTab,true); Tab("Veículos",2,teleportTab,true);
    if (teleportTab == 0) Local::Render(2);
    else World::Render(teleportTab == 1 ? 0 : 1,2);
}
}
