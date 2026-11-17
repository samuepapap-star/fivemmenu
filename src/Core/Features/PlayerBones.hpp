#pragma once
#include <Core/SDK/SDK.hpp>

namespace Core::Features {
class PlayerBones {
    uintptr_t skeleton{}, matrices{}, table{};
    unsigned short count{}, hashed{}, slots{};
    D3DXMATRIX world{};
public:
    bool Open(CPed* ped) {
        // Proba todos os offsets m_FragInst conhecidos para funcionar em qualquer build.
        // Offsets historicos de todos os builds suportados:
        //   0x1430 → builds 2802/2944/3095/3258/3889
        //   0x1450 → builds 2545/2612/2699
        //   0x1400 → fallback padrao
        //   0x13E0 → build 1604
        // O offset bem-sucedido e cacheado (s_cachedOff) para evitar re-probe a cada frame.
        static uintptr_t s_cachedOff = 0;
        static const uintptr_t s_candidates[] = { 0x1430, 0x1450, 0x1400, 0x13E0 };

        auto tryOff = [&](uintptr_t off) -> bool {
            if (!off) return false;
            const auto frag = Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(ped) + off);
            if (!frag) return false;
            const auto instance = Mem.Read<uintptr_t>(frag + 0x68);
            if (!instance) return false;
            const auto sk = Mem.Read<uintptr_t>(instance + 0x178);
            if (!sk) return false;
            const auto data      = Mem.Read<uintptr_t>(sk);
            const auto transform = Mem.Read<uintptr_t>(sk + 0x8);
            const auto mats      = Mem.Read<uintptr_t>(sk + 0x18);
            if (!data || !transform || !mats) return false;
            const auto cnt = Mem.Read<unsigned short>(data + 0x5e);
            const auto hsh = Mem.Read<unsigned short>(data + 0x1a);
            const auto slt = Mem.Read<unsigned short>(data + 0x18);
            const auto tbl = Mem.Read<uintptr_t>(data + 0x10);
            if (!cnt || (hsh && (!slt || !tbl))) return false;
            // tudo valido — salva estado e cacheia offset
            skeleton = sk; matrices = mats;
            world    = Mem.Read<D3DXMATRIX>(transform);
            count    = cnt; hashed = hsh; slots = slt; table = tbl;
            s_cachedOff = off;
            return true;
        };

        // 1) offset cacheado (caminho rapido apos primeiro sucesso)
        if (s_cachedOff && tryOff(s_cachedOff)) return true;
        // 2) offset configurado pelo build detector
        if (g_Offsets.m_FragInst && g_Offsets.m_FragInst != s_cachedOff
                && tryOff(g_Offsets.m_FragInst)) return true;
        // 3) proba todos os candidatos historicos
        for (const auto c : s_candidates)
            if (c != s_cachedOff && c != g_Offsets.m_FragInst && tryOff(c)) return true;
        return false;
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
