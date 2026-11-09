#pragma once
#include <Core/SDK/SDK.hpp>

namespace Core::Features {
class PlayerBones {
    uintptr_t skeleton{}, matrices{}, table{};
    unsigned short count{}, hashed{}, slots{};
    D3DXMATRIX world{};
public:
    bool Open(CPed* ped) {
        if (!g_Offsets.m_FragInst) return false;
        const auto frag = Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(ped) + g_Offsets.m_FragInst);
        if (!frag) return false;
        const auto instance = Mem.Read<uintptr_t>(frag + 0x68);
        if (!instance) return false;
        skeleton = Mem.Read<uintptr_t>(instance + 0x178);
        if (!skeleton) return false;
        const auto data = Mem.Read<uintptr_t>(skeleton);
        const auto transform = Mem.Read<uintptr_t>(skeleton + 0x8);
        matrices = Mem.Read<uintptr_t>(skeleton + 0x18);
        if (!data || !transform || !matrices) return false;
        world = Mem.Read<D3DXMATRIX>(transform);
        count = Mem.Read<unsigned short>(data + 0x5e);
        hashed = Mem.Read<unsigned short>(data + 0x1a);
        slots = Mem.Read<unsigned short>(data + 0x18);
        table = Mem.Read<uintptr_t>(data + 0x10);
        if (!count || (hashed && (!slots || !table))) return false;
        return true;
    }
    bool Position(unsigned mask, D3DXVECTOR3& bone) const {
        unsigned index = mask;
        if (hashed) {
            index = count;
            auto node = Mem.Read<uintptr_t>(table + sizeof(uintptr_t) * (mask % slots));
            for (int depth = 0; node && depth < 3; ++depth) {
                if (Mem.Read<unsigned>(node) == mask) {
                    index = Mem.Read<unsigned>(node + 4);
                    break;
                }
                node = Mem.Read<uintptr_t>(node + 8);
            }
        }
        if (index >= count) return false;
        bone = Core::SDK::Game::GetBonePosInstFrag(skeleton, index, world, matrices);
        return bone != D3DXVECTOR3(0,0,0) && std::isfinite(bone.x) && std::isfinite(bone.y) && std::isfinite(bone.z);
    }
};
}
