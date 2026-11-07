#include "../src/Core/SDK/NativeCandidateLogic.hpp"
#include <array>
#include <cassert>
#include <cstring>
int main() {
    using namespace NativeCandidateLogic;
    std::array<std::uint8_t,12> bytes{0x48,0x8d,0x0d,0,0,0,0,0xe8,0,0,0,0};
    std::uint64_t target=999;std::int32_t displacement=-28;
    std::memcpy(bytes.data()+8,&displacement,4);
    assert(Resolver(bytes.data(),12,32,128,target)&&target==16);
    target=999;assert(!Resolver(bytes.data(),11,32,128,target)&&target==999);
    assert(!Resolver(bytes.data(),12,0,128,target));
    assert(!Resolver(bytes.data(),12,120,128,target));
    assert(!Resolver(bytes.data(),12,32,0xffffffffffffffffULL,target));
    displacement=1000;std::memcpy(bytes.data()+8,&displacement,4);
    assert(!Resolver(bytes.data(),12,32,128,target));
    displacement=0;std::memcpy(bytes.data()+8,&displacement,4);bytes[7]=0x90;
    assert(!Resolver(bytes.data(),12,32,128,target));
    assert(!Pointer(0)&&!Pointer(0xffff)&&!Pointer(0xffffffffffffffffULL)&&Pointer(0x10000));
}

