#include "../src/Core/Features/RollSpinLogic.hpp"
#include <cassert>
int main() {
    RollSpinLogic::Gate gate;
    assert(!gate.Update(true,false,false,100));
    assert(gate.Update(true,false,true,100));
    assert(gate.Update(true,false,false,899));
    assert(!gate.Update(true,false,false,900));
    assert(gate.Update(true,false,true,1000));
    assert(!gate.Update(true,true,false,1001));
    assert(!gate.Update(true,false,false,1002));
    assert(gate.Update(true,false,true,1100));
    assert(!gate.Update(false,false,false,1101));
    assert(!gate.Update(true,false,false,1102));
    const auto maximum=(std::numeric_limits<std::uint64_t>::max)();
    assert(gate.Update(true,false,true,maximum-1));
    assert(gate.deadline==maximum);
    assert(!gate.Update(true,false,false,maximum));
}

