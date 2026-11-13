#pragma once
#include <Windows.h>
#include <array>
#include <string>

namespace MenuShortcut {
inline bool editing = false;
inline int waitRelease = 0;
inline std::array<bool, 255> previous{};

inline bool Down(int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; }

inline void Begin() {
    for (int key = VK_BACK; key < 255; ++key) previous[key] = Down(key);
    editing = true;
}

// Consume the capture press until release so assigning a key never closes the menu.
inline bool Poll(int& configured, bool focused) {
    if (waitRelease) {
        if (!Down(waitRelease)) waitRelease = 0;
        return true;
    }
    if (!editing) return false;
    if (!focused) { editing = false; return true; }
    for (int key = VK_BACK; key < 255; ++key) {
        bool down = Down(key);
        bool pressed = down && !previous[key];
        previous[key] = down;
        if (!pressed) continue;
        if (key != VK_ESCAPE) configured = key;
        editing = false;
        waitRelease = key;
        break;
    }
    return true;
}

inline std::string Label(int key) {
    if (editing) return "Pressione uma tecla (Esc cancela)###menu-shortcut";
    if (!key) return "Alterar tecla: P / Insert###menu-shortcut";
    char name[64]{};
    UINT scan = MapVirtualKeyW(static_cast<UINT>(key), MAPVK_VK_TO_VSC_EX);
    LONG code = static_cast<LONG>((scan & 255) << 16);
    if ((scan & 0xff00) == 0xe000) code |= 1L << 24;
    wchar_t wide[32]{};
    if (GetKeyNameTextW(code, wide, 32))
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, name, sizeof(name), nullptr, nullptr);
    return std::string("Alterar tecla: ") + (name[0] ? name : "Nenhuma") + "###menu-shortcut";
}
}
