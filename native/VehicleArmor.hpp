#pragma once
#include "NativeActions.hpp"
#include <array>

namespace MenuBridge {
class VehicleArmor {
    NativeApi& api;
    struct Protection {
        std::uint64_t id = 0;
        std::array<int, 8> original{};
    } player, vehicle;
    std::uint64_t nextScan = 0;

    bool Read(std::uint64_t id, std::array<int, 8>& flags) {
        std::vector<std::uint64_t> args{id};
        for (auto& flag : flags) args.push_back(reinterpret_cast<std::uint64_t>(&flag));
        return api.Call(0xBE8CD9BE829BBEBFULL, args) != 0;
    }
    void Write(std::uint64_t id, const std::array<int, 8>& flags) {
        std::vector<std::uint64_t> args{id};
        for (auto flag : flags) args.push_back(flag != 0);
        api.Call(0xFAEE099C6F890BB8ULL, args);
    }
    bool Protect(Protection& state, std::uint64_t target) {
        std::array<int, 8> flags{};
        if (state.id && state.id != target) {
            if (api.Call(N::EntityExists, {state.id})) {
                if (!Read(state.id, flags)) return false;
                std::copy_n(state.original.begin(), 3, flags.begin());
                Write(state.id, flags);
                // Remove invencibilidade ao trocar/sair do veiculo
                api.Call(0x3E46A2A9522BB2D0ULL, {state.id, 0}); // SET_VEHICLE_INVINCIBLE false
            }
            state = {};
        }
        if (!target || !Read(target, flags)) return false;
        if (!state.id) state = {target, flags};
        if (!flags[0] || !flags[1] || !flags[2]) {
            std::fill_n(flags.begin(), 3, 1);
            Write(target, flags);
        }
        return true;
    }
public:
    explicit VehicleArmor(NativeApi& native) : api(native) {}
    void Reset() { player = {}; vehicle = {}; nextScan = 0; }
    void Tick(bool enabled, std::uint64_t now) {
        if (!enabled && !vehicle.id && !player.id) return;
        if (enabled && now < nextScan) return;
        nextScan = enabled ? now + 250 : 0;
        auto ped = enabled ? api.Call(N::PlayerPed, {}) : 0;
        auto car = ped ? api.Call(N::CurrentVehicle, {ped, 0}) : 0;
        if (car && api.Call(N::Networked, {car}) && !api.Call(N::HasControl, {car})) car = 0;
        const bool active = Protect(vehicle, car);
        Protect(player, active ? ped : 0);
        if (!active) return;
        // Invencivel: evita fumaca e dano visual
        api.Call(0x3E46A2A9522BB2D0ULL, {car, 1}); // SET_VEHICLE_INVINCIBLE
        for (std::uint64_t window = 0; window < 8; ++window) {
            if (!api.Call(0x46E571A0E20D01F1ULL, {car, window}))
                api.Call(0x772282EBEB95E682ULL, {car, window});
        }
    }
};
}
