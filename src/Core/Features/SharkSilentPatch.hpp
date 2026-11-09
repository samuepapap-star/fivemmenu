#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
namespace SharkSilentPatch {
// SHARK's R9 hook replaces exactly three whole movss instructions (17 bytes).
inline constexpr std::array<std::uint8_t,17> instructions={
    0xF3,0x41,0x0F,0x10,0x19,0xF3,0x41,0x0F,0x10,0x41,0x04,
    0xF3,0x41,0x0F,0x10,0x51,0x08};
inline bool Matches(const std::vector<std::uint8_t>& bytes) {
    return bytes.size()==instructions.size() &&
        std::memcmp(bytes.data(),instructions.data(),instructions.size())==0;
}
inline std::vector<std::uint8_t> Jump(std::uint64_t destination) {
    std::vector<std::uint8_t> patch(17,0x90);
    const std::uint8_t opcode[]={0xFF,0x25,0,0,0,0};
    std::memcpy(patch.data(),opcode,sizeof(opcode));
    std::memcpy(patch.data()+6,&destination,sizeof(destination));
    return patch;
}
inline std::vector<std::uint8_t> Shell(std::uint64_t returnAddress) {
    // Aligned coordinate data at 40/44/48, after the indirect return at 30.
    std::vector<std::uint8_t> bytes={
        0xF3,0x0F,0x10,0x1D,32,0,0,0,
        0xF3,0x0F,0x10,0x05,28,0,0,0,
        0xF3,0x0F,0x10,0x15,24,0,0,0,
        0xFF,0x25,0,0,0,0,0,0,0,0,0,0,0,0,0x90,0x90,
        0,0,0,0,0,0,0,0,0,0,0,0};
    std::memcpy(bytes.data()+30,&returnAddress,sizeof(returnAddress));
    return bytes;
}
template<class Vec> inline bool Sane(const Vec& pos) {
    return std::isfinite(pos.x)&&std::isfinite(pos.y)&&std::isfinite(pos.z)&&
        std::fabs(pos.x)<200000.f&&std::fabs(pos.y)<200000.f&&std::fabs(pos.z)<200000.f;
}
}
