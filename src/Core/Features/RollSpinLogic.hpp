#pragma once
#include <cstdint>
#include <limits>
namespace RollSpinLogic {
// HyperX's Combat Glitch is a Spinbot window after aim + Space, not a separate animation native.
struct Gate {
    std::uint64_t deadline=0;
    void Reset(){deadline=0;}
    bool Update(bool active,bool inAir,bool chord,std::uint64_t now) {
        if(!active||inAir){Reset();return false;}
        if(chord) {
            const auto maximum=(std::numeric_limits<std::uint64_t>::max)();
            deadline=now>maximum-800?maximum:now+800;
        }
        return deadline!=0&&now<deadline;
    }
};
}

