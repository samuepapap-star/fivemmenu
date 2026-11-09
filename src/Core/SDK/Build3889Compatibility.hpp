#pragma once
#include "Build3889Probe.hpp"

namespace Build3889Compatibility {
// Validate the specific executable sampled by the compatibility report before
// allowing feature threads to use its globals. A future 3889 revision must be
// probed again rather than silently using these RVAs.
inline std::string Validate(HANDLE process, uintptr_t base) {
    using namespace Build3889Probe;
    IMAGE_DOS_HEADER dos{};
    IMAGE_NT_HEADERS64 nt{};
    if (!Read(process, base, dos) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew <= 0 || dos.e_lfanew >= 0x100000 ||
        !Read(process, base + dos.e_lfanew, nt) || nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt.OptionalHeader.SizeOfImage != 65387520 || (nt.FileHeader.TimeDateStamp != 1793798832 && nt.FileHeader.TimeDateStamp != 1793965275))
        return "Esta revisao da build 3889 difere do diagnostico. Execute com --diagnostico-3889 e envie o novo relatorio.";

    uintptr_t world=0, ped=0, navigation=0, replay=0, iface=0, list=0, viewport=0, camera=0, follow=0;
    Vec pos{}, navPos{};
    int capacity=0;
    float health=0, maxHealth=0;
    if (!Read(process, base+0x2503A58, world) || !world || !Read(process, world+8, ped) || !ped ||
        !Read(process, ped+0x90, pos) || !Sane(pos) ||
        !Read(process, ped+0x30, navigation) || !navigation ||
        !Read(process, navigation+0x50, navPos) || !Sane(navPos) ||
        !Read(process, ped+0x280, health) || !std::isfinite(health) || health<0 || health>10000 ||
        !Read(process, ped+0x284, maxHealth) || !std::isfinite(maxHealth) || maxHealth<=0 || maxHealth>10000)
        return "Personagem da build 3889 indisponivel. Carregue o personagem na cidade e abra o EXE novamente.";
    if (!Read(process, base+0x1EC2BB8, replay) || !replay ||
        !Read(process, replay+0x18, iface) || !iface ||
        !Read(process, iface+0x100, list) || !list ||
        !Read(process, iface+0x108, capacity) || capacity<=0 || capacity>4096)
        return "Nao foi possivel validar a lista de personagens da build 3889.";
    if (!Read(process, base+0x1F6B320, viewport) || !viewport || !Matrix(process, viewport+0x24C))
        return "Nao foi possivel validar a projecao da build 3889.";
    if (!Read(process, base+0x1F6C480, camera) || !camera ||
        !Read(process, camera+0x2C0, follow) || !follow)
        return "Nao foi possivel validar a camera da build 3889.";
    return {};
}
}
